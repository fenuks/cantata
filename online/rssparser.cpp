/*
 * Cantata
 *
 * Copyright (c) 2011-2022 Craig Drummond <craig.p.drummond@gmail.com>
 *
 * ----
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "rssparser.h"
#include <QXmlStreamReader>
#include <QStringList>
#include <QSet>
#include <QRegularExpression>

static const QString constITunesNameSpace = QStringLiteral("http://www.itunes.com/dtds/podcast-1.0.dtd");
static const QString constMediaNameSpace = QStringLiteral("http://search.yahoo.com/mrss/");

using namespace RssParser;

static bool parseUntil(QXmlStreamReader &reader, const QString &elem)
{
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && reader.name() == elem) {
            return true;
        }
    }
    return false;
}

static void consumeCurrentElement(QXmlStreamReader &reader)
{
    int level = 1;
    while (0!=level && !reader.atEnd()) {
        switch (reader.readNext()) {
            case QXmlStreamReader::StartElement: ++level; break;
            case QXmlStreamReader::EndElement:   --level; break;
            default: break;
        }
    }
}

static QString capitaliseWord(QString str)
{
    if (str.isEmpty() || !str[0].isLetter()) {
        return str;
    }
    str=str.toLower();
    str[0]=str[0].toUpper();
    return str;
}

static QDateTime parseRfc822DateTime(const QString &text)
{
    // This sucks but we need it because some podcasts don't quite follow the
    // spec properly - they might have 1-digit hour numbers for example.

    QRegularExpression re("([a-zA-Z]{3}),? (\\d{1,2}) ([a-zA-Z]{3}) (\\d{4}) (\\d{1,2}):(\\d{1,2}):(\\d{1,2})");
    QRegularExpressionMatch reMatch = re.match(text);
    if (!reMatch.hasMatch()) {
        return QDateTime();
    }

    QDateTime dt(QDate::fromString(QString("%1 %2 %3 %4").arg(reMatch.captured(1), reMatch.captured(3), reMatch.captured(2), reMatch.captured(4)), Qt::TextDate),
                 QTime(reMatch.captured(5).toInt(), reMatch.captured(6).toInt(), reMatch.captured(7).toInt()));

    if (dt.isValid()) {
        return dt;
    }
    return QDateTime(QDate::fromString(QString("%1 %2 %3 %4").arg(capitaliseWord(reMatch.captured(1)), capitaliseWord(reMatch.captured(3)), reMatch.captured(2), reMatch.captured(4)), Qt::TextDate),
                     QTime(reMatch.captured(5).toInt(), reMatch.captured(6).toInt(), reMatch.captured(7).toInt()));
}

static QUrl parseImage(QXmlStreamReader &reader)
{
    QUrl url;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (QLatin1String("url")==reader.name()) {
                url=QUrl::fromEncoded(reader.readElementText().toLatin1());
            } else {
                consumeCurrentElement(reader);
            }
        } else if (reader.isEndElement()) {
            break;
        }
    }
    return url;
}

static Episode parseEpisode(QXmlStreamReader &reader)
{
    Episode ep;
    bool isAudio=false;
    QUrl guidUrl;

    while (!reader.atEnd()) {
        reader.readNext();
        const QStringView name = reader.name();
        if (reader.isStartElement()) {
            if (QLatin1String("title")==name) {
                ep.name=reader.readElementText().simplified();
            } else if (QLatin1String("duration")==name && constITunesNameSpace==reader.namespaceUri()) {
                QStringList parts = reader.readElementText().split(':');
                if (2==parts.count()) {
                    ep.duration=(parts[0].toInt() * 60) + parts[1].toInt();
                } else if (parts.count()>=3) {
                    ep.duration=(parts[0].toInt() * 60*60) + (parts[1].toInt() * 60) + parts[2].toInt();
                }
            } else if (0==ep.duration && QLatin1String("content")==name && constMediaNameSpace==reader.namespaceUri()) {
                ep.duration=reader.attributes().value(QLatin1String("duration")).toString().toUInt();
                consumeCurrentElement(reader);
            } else if (QLatin1String("enclosure")==name) {
                static QSet<QString> audioFormats;
                if (audioFormats.isEmpty()) {
                    audioFormats.insert(QLatin1String("mp3")); audioFormats.insert(QLatin1String("MP3"));
                    audioFormats.insert(QLatin1String("ogg")); audioFormats.insert(QLatin1String("OGG"));
                    audioFormats.insert(QLatin1String("wma")); audioFormats.insert(QLatin1String("WMA"));
                }
                QString type=reader.attributes().value(QLatin1String("type")).toString();
                if (type.startsWith(QLatin1String("audio/")) || audioFormats.contains(type)) {
                    isAudio=true;
                    ep.url=QUrl::fromEncoded(reader.attributes().value(QLatin1String("url")).toString().toLatin1());
                } else if (type.startsWith(QLatin1String("video/")) ) {
                    // At least one broken feed (BUG: 588) has the audio podcast listed as video/mp4,
                    // ...but the path ends in .mp3 !!!
                    QUrl url=QUrl::fromEncoded(reader.attributes().value(QLatin1String("url")).toString().toLatin1());
                    QString path=url.path();
                    if (path.endsWith(QLatin1String(".mp3"), Qt::CaseInsensitive) ||
                        path.endsWith(QLatin1String(".ogg"), Qt::CaseInsensitive) ||
                        path.endsWith(QLatin1String(".wma"), Qt::CaseInsensitive)) {
                        ep.url=url;
                    } else {
                        ep.video=true;
                    }
                }
                consumeCurrentElement(reader);
            } else if (QLatin1String("guid")==name) {
                guidUrl=QUrl(reader.readElementText());
            } else if (QLatin1String("pubDate")==name) {
                 ep.publicationDate=parseRfc822DateTime(reader.readElementText());
            } else if (QLatin1String("description")==name) {
                ep.description=reader.readElementText();
            } else {
                consumeCurrentElement(reader);
            }
        } else if (reader.isEndElement()) {
            break;
        }
    }

    // Sometimes the url entry in 'enclusure' is empty, but there is a url in 'guid' - so use
    // that if present (BUG: 602)
    if (isAudio && ep.url.isEmpty() && !guidUrl.isEmpty()) {
        ep.url=guidUrl;
    }
    return ep;
}

Channel RssParser::parse(QIODevice *dev, bool getEpisodes, bool getDescription)
{
    Channel ch;
    QXmlStreamReader reader(dev);
    if (parseUntil(reader, QLatin1String("rss")) && parseUntil(reader, QLatin1String("channel"))) {
        while (!reader.atEnd()) {
            reader.readNext();

            if (reader.isStartElement()) {
                const QStringView name = reader.name();
                if (ch.name.isEmpty() && QLatin1String("title")==name) {
                    ch.name=reader.readElementText().simplified();
                } else if (QLatin1String("image")==name && ch.image.isEmpty()) {
                    if (constITunesNameSpace==reader.namespaceUri()) {
                        ch.image=reader.attributes().value(QLatin1String("href")).toString();
                        consumeCurrentElement(reader);
                    } else {
                        ch.image=parseImage(reader);
                    }
                } else if (getEpisodes && QLatin1String("item")==name) {
                    Episode ep=parseEpisode(reader);
                    if (!ep.name.isEmpty() && !ep.url.isEmpty()) {
                        ch.episodes.append(ep);
                    } else if (ep.video) {
                        ch.video=true;
                    }
                } else if (getDescription && QLatin1String("description")==name && ch.description.isEmpty()) {
                    ch.description=reader.readElementText();
                } else if (getDescription && QLatin1String("summary")==name && ch.description.isEmpty() && constITunesNameSpace==reader.namespaceUri()) {
                    ch.description=reader.readElementText();
                } else {
                    consumeCurrentElement(reader);
                }
            } else if (reader.isEndElement()) {
                break;
            }
        }
    }

    if (ch.video && !ch.episodes.isEmpty()) {
        ch.video=false;
    }
    return ch;
}
