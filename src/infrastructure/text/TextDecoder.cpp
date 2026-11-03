#include "infrastructure/text/TextDecoder.h"

#include <QFile>
#include <QRegularExpression>
#include <QXmlStreamReader>

#include <algorithm>

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

quint16 readBigEndian16(const QByteArray &data, qsizetype offset)
{
    return offset + 1 < data.size()
        ? (static_cast<quint16>(static_cast<uchar>(data.at(offset))) << 8)
              | static_cast<quint16>(static_cast<uchar>(data.at(offset + 1)))
        : 0;
}

quint32 readBigEndian32(const QByteArray &data, qsizetype offset)
{
    if (offset + 3 >= data.size()) {
        return 0;
    }
    return (static_cast<quint32>(static_cast<uchar>(data.at(offset))) << 24)
        | (static_cast<quint32>(static_cast<uchar>(data.at(offset + 1))) << 16)
        | (static_cast<quint32>(static_cast<uchar>(data.at(offset + 2))) << 8)
        | static_cast<quint32>(static_cast<uchar>(data.at(offset + 3)));
}

QByteArray decompressPalmDoc(const QByteArray &compressed)
{
    QByteArray output;
    output.reserve(compressed.size());
    for (qsizetype index = 0; index < compressed.size(); ++index) {
        const uchar value = static_cast<uchar>(compressed.at(index));
        if (value >= 0xC0) {
            output.append(' ');
            output.append(static_cast<char>(value ^ 0x80));
        } else if (value >= 0x80) {
            if (index + 1 >= compressed.size()) {
                return {};
            }
            const uchar next = static_cast<uchar>(compressed.at(++index));
            const int distance = ((value & 0x3F) << 5) | (next >> 3);
            const int length = (next & 0x07) + 3;
            if (distance <= 0 || distance > output.size()) {
                return {};
            }
            for (int copy = 0; copy < length; ++copy) {
                output.append(output.at(output.size() - distance));
            }
        } else {
            output.append(static_cast<char>(value));
        }
    }
    return output;
}

QString decodePalmDocument(const QByteArray &data)
{
    constexpr qsizetype minimumHeaderSize = 86;
    if (data.size() < minimumHeaderSize) {
        return {};
    }

    const quint16 recordCount = readBigEndian16(data, 76);
    const quint32 firstRecordOffset = readBigEndian32(data, 78);
    if (recordCount == 0 || firstRecordOffset < minimumHeaderSize
        || firstRecordOffset >= static_cast<quint32>(data.size())) {
        return {};
    }

    QList<quint32> offsets;
    offsets.reserve(recordCount);
    for (quint16 index = 0; index < recordCount; ++index) {
        const qsizetype recordInfoOffset = 78 + static_cast<qsizetype>(index) * 8;
        if (recordInfoOffset + 3 >= data.size()) {
            return {};
        }
        offsets.append(readBigEndian32(data, recordInfoOffset));
    }
    offsets.append(static_cast<quint32>(data.size()));

    const QByteArray firstRecord = data.mid(offsets.at(0), offsets.at(1) - offsets.at(0));
    if (firstRecord.size() < 16) {
        return {};
    }
    const quint16 compression = readBigEndian16(firstRecord, 0);
    const quint16 textRecordCount = readBigEndian16(firstRecord, 8);
    const quint16 encryption = readBigEndian16(firstRecord, 12);
    if (encryption != 0 || (compression != 1 && compression != 2) || textRecordCount == 0) {
        return {};
    }

    qsizetype textOffset = 16;
    if (firstRecord.size() >= textOffset + 24
        && firstRecord.mid(textOffset, 4) == QByteArrayLiteral("MOBI")) {
        const quint32 mobiHeaderLength = readBigEndian32(firstRecord, textOffset + 4);
        if (mobiHeaderLength < 16 || textOffset + mobiHeaderLength > firstRecord.size()) {
            return {};
        }
        const quint32 exthFlags = firstRecord.size() >= textOffset + 132
            ? readBigEndian32(firstRecord, textOffset + 128)
            : 0;
        textOffset += mobiHeaderLength;
        if ((exthFlags & 0x40U) != 0 && firstRecord.size() >= textOffset + 8
            && firstRecord.mid(textOffset, 4) == QByteArrayLiteral("EXTH")) {
            const quint32 exthLength = readBigEndian32(firstRecord, textOffset + 4);
            if (exthLength < 12 || textOffset + exthLength > firstRecord.size()) {
                return {};
            }
            textOffset += exthLength;
        }
    }

    QByteArray text;
    const int recordsToRead = std::min<int>(textRecordCount, offsets.size() - 1);
    for (int index = 0; index < recordsToRead; ++index) {
        const qsizetype start = offsets.at(index);
        const qsizetype end = offsets.at(index + 1);
        if (start < 0 || end < start || end > data.size()) {
            return {};
        }
        QByteArray record = data.mid(start, end - start);
        if (index == 0) {
            if (textOffset > record.size()) {
                return {};
            }
            record = record.mid(textOffset);
        }
        const QByteArray decoded = compression == 2 ? decompressPalmDoc(record) : record;
        if (decoded.isEmpty() && !record.isEmpty()) {
            return {};
        }
        text.append(decoded);
    }
    return QString::fromUtf8(text).trimmed();
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
    if (format.compare(QStringLiteral("mobi"), Qt::CaseInsensitive) == 0
        || format.compare(QStringLiteral("azw"), Qt::CaseInsensitive) == 0
        || format.compare(QStringLiteral("azw3"), Qt::CaseInsensitive) == 0) {
        return decodePalmDocument(data);
    }
    if (format.compare(QStringLiteral("rtf"), Qt::CaseInsensitive) == 0) {
        return decodeRtf(QString::fromUtf8(data));
    }
    if (format.compare(QStringLiteral("fb2"), Qt::CaseInsensitive) == 0) {
        return decodeFb2(data);
    }
    return QString::fromUtf8(data);
}

} // namespace Lumina::Infrastructure
