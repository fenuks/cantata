/*
 * Cantata
 *
 * Copyright (c) 2011-2022 Craig Drummond <craig.p.drummond@gmail.com>
 *
 */
/* This file is part of Clementine.
   Copyright 2010, David Sansome <me@davidsansome.com>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ultimatelyrics.h"
#include "ultimatelyricsprovider.h"
#include "gui/settings.h"
#include "support/globalstatic.h"
#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QXmlStreamReader>
#include <QSet>
#include <algorithm>

GLOBAL_STATIC(UltimateLyrics, instance)

static bool compareLyricProviders(const UltimateLyricsProvider *a, const UltimateLyricsProvider *b)
{
    return a->getRelevance() < b->getRelevance();
}

static QString parseInvalidIndicator(QXmlStreamReader *reader)
{
    QString ret = reader->attributes().value("value").toString();
    reader->skipCurrentElement();
    return ret;
}

static UltimateLyricsProvider::Rule parseRule(QXmlStreamReader *reader)
{
    UltimateLyricsProvider::Rule ret;

    while (!reader->atEnd()) {
        reader->readNext();

        if (QXmlStreamReader::EndElement==reader->tokenType()) {
            break;
        }

        if (QXmlStreamReader::StartElement==reader->tokenType()) {
            if (QLatin1String("item")==reader->name()) {
                QXmlStreamAttributes attr = reader->attributes();
                if (attr.hasAttribute("tag")) {
                    ret << UltimateLyricsProvider::RuleItem(attr.value("tag").toString(), QString());
                } else if (attr.hasAttribute("begin")) {
                    ret << UltimateLyricsProvider::RuleItem(attr.value("begin").toString(), attr.value("end").toString());
                }
            }
            reader->skipCurrentElement();
        }
    }
    return ret;
}

static UltimateLyricsProvider * parseProvider(QXmlStreamReader *reader)
{
    QXmlStreamAttributes attributes = reader->attributes();

    UltimateLyricsProvider* scraper = new UltimateLyricsProvider;
    scraper->setName(attributes.value("name").toString());
    scraper->setCharset(attributes.value("charset").toString());
    scraper->setUrl(attributes.value("url").toString());

    while (!reader->atEnd()) {
        reader->readNext();

        if (QXmlStreamReader::EndElement==reader->tokenType()) {
            break;
        }

        if (QXmlStreamReader::StartElement==reader->tokenType()) {
            if (QLatin1String("extract")==reader->name()) {
                scraper->addExtractRule(parseRule(reader));
            } else if (QLatin1String("exclude")==reader->name()) {
                scraper->addExcludeRule(parseRule(reader));
            } else if (QLatin1String("invalidIndicator")==reader->name()) {
                scraper->addInvalidIndicator(parseInvalidIndicator(reader));
            } else if (QLatin1String("urlFormat")==reader->name()) {
                scraper->addUrlFormat(reader->attributes().value("replace").toString(), reader->attributes().value("with").toString());
                reader->skipCurrentElement();
            } else {
                reader->skipCurrentElement();
            }
        }
    }
    return scraper;
}

void UltimateLyrics::release()
{
    for (UltimateLyricsProvider *provider: providers) {
        delete provider;
    }
    providers.clear();
}

const QList<UltimateLyricsProvider *> UltimateLyrics::getProviders()
{
    load();
    return providers;
}

UltimateLyricsProvider * UltimateLyrics::providerByName(const QString &name) const
{
    for (UltimateLyricsProvider *provider: providers) {
        if (provider->getName() == name) {
            return provider;
        }
    }
    return nullptr;
}

UltimateLyricsProvider * UltimateLyrics::getNext(int &index)
{
    load();
    index++;
    if (index>-1 && index<providers.count()) {
        for (int i=index; i<providers.count(); ++i) {
            if (providers.at(i)->isEnabled()) {
                index=i;
                return providers.at(i);
            }
        }
    }
    return nullptr;
}

void UltimateLyrics::load()
{
    if (!providers.isEmpty()) {
        return;
    }

    QStringList files;
    QString userDir=Utils::dataDir();

    if (!userDir.isEmpty()) {
        QFileInfoList files=QDir(userDir).entryInfoList(QStringList() << QLatin1String("lyrics_*.xml"), QDir::NoDotAndDotDot|QDir::Files);
        for (const QFileInfo &f: files) {
            files.append(QFileInfo(f.absoluteFilePath()));
        }
    }

    files.append(":lyrics_providers.xml");

    QSet<QString> providerNames;

    for (const auto &f: files) {
        QFile file(f);
        if (file.open(QIODevice::ReadOnly)) {
            QXmlStreamReader reader(&file);
            while (!reader.atEnd()) {
                reader.readNext();

                if (QLatin1String("provider")==reader.name()) {
                    QString name=reader.attributes().value("name").toString();

                    if (!providerNames.contains(name)) {
                        UltimateLyricsProvider *provider = parseProvider(&reader);
                        if (provider) {
                            providers << provider;
                            connect(provider, SIGNAL(lyricsReady(int,QString)), this, SIGNAL(lyricsReady(int,QString)));
                            providerNames.insert(name);
                        }
                    }
                }
            }
        }
    }

    setEnabled(Settings::self()->lyricProviders());
}

void UltimateLyrics::setEnabled(const QStringList &enabled)
{
    for (UltimateLyricsProvider *provider: providers) {
        provider->setEnabled(false);
        provider->setRelevance(0xFFFF);
    }

    int relevance=0;
    for (const QString &p: enabled) {
        UltimateLyricsProvider *provider=providerByName(p);
        if (provider) {
            provider->setEnabled(true);
            provider->setRelevance(relevance++);
        }
    }
    std::sort(providers.begin(), providers.end(), compareLyricProviders);
    Settings::self()->saveLyricProviders(enabled);
}

#include "moc_ultimatelyrics.cpp"
