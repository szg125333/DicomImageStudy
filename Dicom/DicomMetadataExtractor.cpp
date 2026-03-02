#include "DicomMetadataExtractor.h"
#include <dcmtk/dcmdata/dctk.h>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

// 辅助函数：安全获取标签值
static QString getTagValue(DcmDataset* dataset, Uint16 group, Uint16 elem)
{
    OFString value;
    if (dataset->findAndGetOFString(DcmTagKey(group, elem), value).good()) {
        std::string str = value.c_str();
        // 移除尾部空格（DICOM 字符串常用填充）
        if (!str.empty()) {
            str.erase(str.find_last_not_of(' ') + 1);
        }
        return QString::fromStdString(str);
    }
    return QString();
}

// 辅助函数：处理多值标签（如 Pixel Spacing）
static QString getMultiValueTag(DcmDataset* dataset, Uint16 group, Uint16 elem, char separator = ' ')
{
    OFString value;
    if (dataset->findAndGetOFString(DcmTagKey(group, elem), value).good()) {
        std::string str = value.c_str();
        // 替换反斜杠为分隔符（DICOM 多值用 \ 分隔）
        std::replace(str.begin(), str.end(), '\\', separator);
        return QString::fromStdString(str);
    }
    return QString();
}

QMap<QString, QString> DicomMetadataExtractor::extractFromFile(const QString& filePath)
{
    QMap<QString, QString> metadata;

    DcmFileFormat fileformat;
    OFCondition status = fileformat.loadFile(filePath.toLocal8Bit().constData());
    if (!status.good()) {
        qWarning() << "Failed to load DICOM file:" << filePath << status.text();
        return metadata;
    }

    DcmDataset* dataset = fileformat.getDataset();
    if (!dataset) return metadata;

    // 定义标签列表（使用 std::make_pair 确保兼容性）
    const std::vector<std::pair<QString, std::pair<Uint16, Uint16>>> tags = {
        std::make_pair(QString("Patient Name"), std::make_pair(0x0010, 0x0010)),
        std::make_pair(QString("Patient ID"), std::make_pair(0x0010, 0x0020)),
        std::make_pair(QString("Patient Birth Date"), std::make_pair(0x0010, 0x0030)),
        std::make_pair(QString("Patient Sex"), std::make_pair(0x0010, 0x0040)),
        std::make_pair(QString("Patient Age"), std::make_pair(0x0010, 0x1010)),
        std::make_pair(QString("Study Date"), std::make_pair(0x0008, 0x0020)),
        std::make_pair(QString("Study Time"), std::make_pair(0x0008, 0x0030)),
        std::make_pair(QString("Modality"), std::make_pair(0x0008, 0x0060)),
        std::make_pair(QString("Study Description"), std::make_pair(0x0008, 0x1030)),
        std::make_pair(QString("Series Description"), std::make_pair(0x0008, 0x103E)),
        std::make_pair(QString("Body Part Examined"), std::make_pair(0x0018, 0x0015)),
        std::make_pair(QString("Protocol Name"), std::make_pair(0x0018, 0x1030)),
        std::make_pair(QString("Institution Name"), std::make_pair(0x0008, 0x0080)),
        std::make_pair(QString("Manufacturer"), std::make_pair(0x0008, 0x0070)),
        std::make_pair(QString("Manufacturer Model Name"), std::make_pair(0x0008, 0x1090)),
        std::make_pair(QString("Slice Thickness (mm)"), std::make_pair(0x0018, 0x0050)),
        std::make_pair(QString("KVP"), std::make_pair(0x0018, 0x0060)),
        std::make_pair(QString("X-Ray Tube Current (mA)"), std::make_pair(0x0018, 0x1150)),
        std::make_pair(QString("Exposure Time (ms)"), std::make_pair(0x0018, 0x1151)),
        std::make_pair(QString("Convolution Kernel"), std::make_pair(0x0018, 0x1210)),
        std::make_pair(QString("Rows"), std::make_pair(0x0028, 0x0010)),
        std::make_pair(QString("Columns"), std::make_pair(0x0028, 0x0011)),
        std::make_pair(QString("Number of Frames"), std::make_pair(0x0028, 0x0008)),
        std::make_pair(QString("Series Number"), std::make_pair(0x0020, 0x0011)),
        std::make_pair(QString("Instance Number"), std::make_pair(0x0020, 0x0013)),
        std::make_pair(QString("Image Type"), std::make_pair(0x0008, 0x0008)),
        std::make_pair(QString("Patient Position"), std::make_pair(0x0018, 0x5100)),
        std::make_pair(QString("Station Name"), std::make_pair(0x0008, 0x1010)),
        std::make_pair(QString("Device Serial Number"), std::make_pair(0x0018, 0x1000)),
    };

    // 提取单值标签
    for (const auto& tag : tags) {
        QString value = getTagValue(dataset, tag.second.first, tag.second.second);
        if (!value.isEmpty()) {
            metadata[tag.first] = value;
        }
    }

    // 特殊处理多值标签
    QString pixelSpacing = getMultiValueTag(dataset, 0x0028, 0x0030, 'x');
    if (!pixelSpacing.isEmpty()) {
        metadata["Pixel Spacing (mm)"] = pixelSpacing;
    }

    QString imagePosition = getMultiValueTag(dataset, 0x0020, 0x0032, ',');
    if (!imagePosition.isEmpty()) {
        metadata["Image Position Patient"] = imagePosition;
    }

    QString imageOrientation = getMultiValueTag(dataset, 0x0020, 0x0037, ',');
    if (!imageOrientation.isEmpty()) {
        metadata["Image Orientation Patient"] = imageOrientation;
    }

    // 添加文件路径信息
    QFileInfo fileInfo(filePath);
    metadata["File Name"] = fileInfo.fileName();
    metadata["Directory"] = fileInfo.absolutePath();

    return metadata;
}

QMap<QString, QString> DicomMetadataExtractor::extractFromDirectory(const QString& dirPath)
{
    QDir dir(dirPath);
    QStringList filters;
    filters << "*.dcm" << "*.DCM" << "*";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    if (files.isEmpty()) {
        qWarning() << "No DICOM files found in directory:" << dirPath;
        return {};
    }
    return extractFromFile(files.first().absoluteFilePath());
}