#pragma once

#include <QMap>
#include <QString>
#include <vector>

class DicomMetadataExtractor
{
public:
    // 从单个 DICOM 文件提取元数据
    static QMap<QString, QString> extractFromFile(const QString& filePath);

    // 从文件夹中任选一个文件（通常第一个）提取
    static QMap<QString, QString> extractFromDirectory(const QString& dirPath);
};