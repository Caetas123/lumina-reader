#include "infrastructure/text/TextDecoder.h"

#include <QFile>
#include <QRegularExpression>
#include <QXmlStreamReader>

namespace {

QString decodeRtf(const QString &source)
{
    QString text;
    text.reserve(source.size());
    bool escaped = false;
    bool controlWord = false;
    bool controlParameter = false;
    QString control;

    for (qsizetype index = 0; index < source.size(); ++index) {
        const QChar character = source.at(index);
        if (escaped) {
            escaped = false;
            if (character == QChar('~')) {
                text.append(QChar(' '));
            } else if (character == QChar('-') || character == QChar('_')) {
                text.append(QChar('-'));
            } else if (character == QChar('\\') || character == QChar('{') || character == QChar('}')) {
                text.append(character);
            } else if (character == QChar('\'')) {
                if (index + 2 < source.size()) {
                    bool ok = false;
                    const ushort code = source.mid(index + 1, 2).toUShort(&ok, 16);
                    if (ok) {
                        text.append(QChar::fromLatin1(static_cast<char>(code)));
                        index += 2;
                    }
                }
            } else {
                controlWord = character.isLetter();
                control = character.isLetter() ? QString(character) : QString();
                controlParameter = false;
                if (!controlWord && character == QChar('~')) {
                    text.append(QChar(' '));
                }
            }
            continue;
        }

        if (character == QChar('\\')) {
            escaped = true;
            controlWord = false;
            controlParameter = false;
            control.clear();
            continue;
        }
        if (controlWord) {
            if (character.isLetter()) {
                control.append(character);
                continue;
            }
            if (character.isDigit() || (character == QChar('-') && index + 1 < source.size()
                                       && source.at(index + 1).isDigit())) {
                controlParameter = true;
                continue;
            }
            if (controlParameter) {
                controlParameter = false;
                if (character == QChar(' ')) {
                    continue;
                }
                if (character == QChar('\\')) {
                    escaped = true;
                    controlWord = false;
                    control.clear();
                    continue;
                }
            }
            if (control == QStringLiteral("par") || control == QStringLiteral("line")) {
                text.append(QChar('\n'));
            } else if (control == QStringLiteral("tab")) {
                text.append(QChar('\t'));
            }
            controlWord = false;
            control.clear();
            if (character == QChar(' ')) {
                continue;
            }
        }

        if (character != QChar('{') && character != QChar('}')) {
            text.append(character);
        }
    }

    text.replace(QRegularExpression(QStringLiteral("[ \\t]+\\n")), QStringLiteral("\n"));
    text.replace(QRegularExpression(QStringLiteral("\\n{3,}")), QStringLiteral("\n\n"));
    return text.trimmed();
}

QString decodeFb2(const QByteArray &data)
{
    QXmlStreamReader reader(data);
    QString text;
    bool inBody = false;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            const QString name = reader.name().toString().toLower();
            if (name == QStringLiteral("body")) {
                inBody = true;
            } else if (inBody && (name == QStringLiteral("p") || name == QStringLiteral("title")
                                  || name == QStringLiteral("subtitle"))) {
                if (!text.isEmpty() && !text.endsWith(QChar('\n'))) {
                    text.append(QChar('\n'));
                }
            }
        } else if (reader.isCharacters() && inBody) {
            text.append(reader.text().toString());
        } else if (reader.isEndElement() && reader.name().toString().compare(QStringLiteral("body"), Qt::CaseInsensitive) == 0) {
            inBody = false;
        }
    }
    return text.trimmed();
}

} // namespace

namespace Lumina::Infrastructure {

QString TextDecoder::readFile(const QString &path, const QString &format)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QByteArray data = file.readAll();
    if (format.compare(QStringLiteral("rtf"), Qt::CaseInsensitive) == 0) {
        return decodeRtf(QString::fromUtf8(data));
    }
    if (format.compare(QStringLiteral("fb2"), Qt::CaseInsensitive) == 0) {
        return decodeFb2(data);
    }
    return QString::fromUtf8(data);
}

} // namespace Lumina::Infrastructure
