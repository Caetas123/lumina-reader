#pragma once

#include <QImage>
#include <QString>

namespace Lumina::Infrastructure {

// Loads formats covered by Qt's image plugins and, when compiled in, WebP/AVIF
// through their native codecs. The caller receives a fully-owned QImage.
QImage loadImageFile(const QString &filePath);

bool canReadImageFile(const QString &filePath);

} // namespace Lumina::Infrastructure
