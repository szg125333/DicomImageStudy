#include "RtStructReader.h"

// GDCM
#include <gdcmReader.h>
#include <gdcmFile.h>
#include <gdcmDataSet.h>
#include <gdcmTag.h>
#include <gdcmAttribute.h>
#include <gdcmSequenceOfItems.h>
#include <gdcmItem.h>
#include <gdcmSmartPointer.h>
#include <gdcmMediaStorage.h>
#include <gdcmGlobal.h>
#include <gdcmDicts.h>
#include <gdcmStringFilter.h>

#include <QDir>
#include <QFileInfoList>
#include <QDebug>

#include <sstream>
#include <algorithm>
#include <cmath>

// ============================================================
//  辅助：从 DataSet 中安全读取字符串 Tag
// ============================================================

static std::string GetTagValue(const gdcm::DataSet& ds,
    uint16_t group, uint16_t element)
{
    gdcm::Tag tag(group, element);
    if (!ds.FindDataElement(tag)) return "";

    const gdcm::DataElement& de = ds.GetDataElement(tag);
    if (de.IsEmpty()) return "";

    const gdcm::ByteValue* bv = de.GetByteValue();
    if (!bv) return "";

    std::string val(bv->GetPointer(), bv->GetLength());
    // 去掉末尾空白/null
    while (!val.empty() && (val.back() == ' ' || val.back() == '\0'))
        val.pop_back();
    return val;
}

// ============================================================
//  ParseColor："R\G\B"（0~255）→ array<double,3>（0~1）
// ============================================================

std::array<double, 3> RtStructReader::ParseColor(const std::string& s)
{
    std::array<double, 3> c = { 1.0, 0.0, 0.0 };
    if (s.empty()) return c;

    std::istringstream ss(s);
    std::string token;
    int idx = 0;
    while (std::getline(ss, token, '\\') && idx < 3) {
        try { c[idx] = std::stod(token) / 255.0; }
        catch (...) {}
        ++idx;
    }
    return c;
}

// ============================================================
//  ParsePoints："x1\y1\z1\x2\y2\z2\..." → 点列表
// ============================================================

std::vector<std::array<double, 3>> RtStructReader::ParsePoints(const std::string& s)
{
    std::vector<std::array<double, 3>> pts;
    if (s.empty()) return pts;

    std::istringstream ss(s);
    std::string token;
    std::vector<double> vals;

    while (std::getline(ss, token, '\\')) {
        try { vals.push_back(std::stod(token)); }
        catch (...) {}
    }

    const size_t n = vals.size();
    pts.reserve(n / 3);
    for (size_t i = 0; i + 2 < n; i += 3) {
        pts.push_back({ vals[i], vals[i + 1], vals[i + 2] });
    }
    return pts;
}

// ============================================================
//  Read — 解析 RS 文件
// ============================================================

RtStructData RtStructReader::Read(const std::string& rsFilePath)
{
    RtStructData result;
    result.filePath = rsFilePath;

    // ── 读取 DICOM 文件 ──────────────────────────────────────────
    gdcm::Reader reader;
    reader.SetFileName(rsFilePath.c_str());
    if (!reader.Read()) {
        qDebug() << "[RtStructReader] Failed to read file:" << rsFilePath.c_str();
        return result;
    }

    const gdcm::File& file = reader.GetFile();
    const gdcm::DataSet& ds = file.GetDataSet();

    // ── Step 1：读取 StructureSetROISequence (3006,0020) ────────
    // 建立 roiNumber → roiName 的映射
    std::map<int, std::string> roiNames;
    {
        gdcm::Tag seqTag(0x3006, 0x0020);
        if (ds.FindDataElement(seqTag)) {
            const gdcm::DataElement& seqDE = ds.GetDataElement(seqTag);
            gdcm::SmartPointer<gdcm::SequenceOfItems> seq =
                seqDE.GetValueAsSQ();
            if (seq) {
                for (unsigned int i = 1; i <= seq->GetNumberOfItems(); ++i) {
                    const gdcm::Item& item = seq->GetItem(i);
                    const gdcm::DataSet& itemDS = item.GetNestedDataSet();

                    // (3006,0022) ROI Number
                    std::string numStr = GetTagValue(itemDS, 0x3006, 0x0022);
                    // (3006,0026) ROI Name
                    std::string name = GetTagValue(itemDS, 0x3006, 0x0026);

                    if (!numStr.empty()) {
                        try {
                            roiNames[std::stoi(numStr)] = name;
                        }
                        catch (...) {}
                    }
                }
            }
        }
    }

    // ── Step 2：读取 ROIContourSequence (3006,0039) ──────────────
    {
        gdcm::Tag seqTag(0x3006, 0x0039);
        if (!ds.FindDataElement(seqTag)) {
            qDebug() << "[RtStructReader] No ROIContourSequence found.";
            return result;
        }

        const gdcm::DataElement& seqDE = ds.GetDataElement(seqTag);
        gdcm::SmartPointer<gdcm::SequenceOfItems> seq = seqDE.GetValueAsSQ();
        if (!seq) return result;

        for (unsigned int roiIdx = 1; roiIdx <= seq->GetNumberOfItems(); ++roiIdx) {
            const gdcm::Item& roiItem = seq->GetItem(roiIdx);
            const gdcm::DataSet& roiDS = roiItem.GetNestedDataSet();

            RoiContour roi;

            // (3006,0084) Referenced ROI Number
            std::string refNum = GetTagValue(roiDS, 0x3006, 0x0084);
            if (!refNum.empty()) {
                try {
                    roi.roiNumber = std::stoi(refNum);
                }
                catch (...) {}
            }

            // ROI 名称从 Step1 的映射里取
            auto it = roiNames.find(roi.roiNumber);
            roi.roiName = (it != roiNames.end()) ? it->second : ("ROI_" + refNum);

            // (3006,002A) ROI Display Color
            std::string colorStr = GetTagValue(roiDS, 0x3006, 0x002A);
            roi.color = ParseColor(colorStr);

            // (3006,0040) ContourSequence ────────────────────────
            gdcm::Tag contourSeqTag(0x3006, 0x0040);
            if (!roiDS.FindDataElement(contourSeqTag)) {
                qDebug() << "[RtStructReader] ROI" << roi.roiName.c_str()
                    << "has no ContourSequence, skipped.";
                continue;
            }

            const gdcm::DataElement& contourSeqDE = roiDS.GetDataElement(contourSeqTag);
            gdcm::SmartPointer<gdcm::SequenceOfItems> contourSeq =
                contourSeqDE.GetValueAsSQ();
            if (!contourSeq) continue;

            for (unsigned int cIdx = 1; cIdx <= contourSeq->GetNumberOfItems(); ++cIdx) {
                const gdcm::Item& cItem = contourSeq->GetItem(cIdx);
                const gdcm::DataSet& cDS = cItem.GetNestedDataSet();

                // (3006,0042) ContourGeometricType — 只处理平面闭合轮廓
                std::string geomType = GetTagValue(cDS, 0x3006, 0x0042);
                // 去掉空格后比较
                geomType.erase(std::remove(geomType.begin(), geomType.end(), ' '),
                    geomType.end());
                if (geomType != "CLOSED_PLANAR") continue;

                // (3006,0050) ContourData
                std::string contourData = GetTagValue(cDS, 0x3006, 0x0050);
                auto pts = ParsePoints(contourData);
                if (pts.empty()) continue;

                ContourSlice slice;
                // 取第一个点的 Z 作为切片坐标（平面轮廓所有点 Z 值相同）
                slice.sliceZ = pts[0][2];
                slice.points = std::move(pts);
                roi.slices.push_back(std::move(slice));
            }

            if (!roi.IsEmpty()) {
                // 按切片 Z 排序
                std::sort(roi.slices.begin(), roi.slices.end(),
                    [](const ContourSlice& a, const ContourSlice& b) {
                        return a.sliceZ < b.sliceZ;
                    });
                result.rois.push_back(std::move(roi));
                qDebug() << "[RtStructReader] ROI:" << result.rois.back().roiName.c_str()
                    << "slices:" << result.rois.back().slices.size();
            }
        }
    }

    qDebug() << "[RtStructReader] Total ROIs loaded:" << result.rois.size();
    return result;
}

// ============================================================
//  FindRsFile — 在文件夹中查找 RTSTRUCT 文件
// ============================================================

std::string RtStructReader::FindRsFile(const std::string& dicomFolder)
{
    QDir dir(QString::fromStdString(dicomFolder));
    const auto entries = dir.entryInfoList(
        QStringList() << "*.dcm" << "*.DCM",
        QDir::Files);

    for (const QFileInfo& fi : entries) {
        std::string path = fi.absoluteFilePath().toStdString();

        gdcm::Reader reader;
        reader.SetFileName(path.c_str());
        // 只读 meta 头，速度快
        if (!reader.ReadUpToTag(gdcm::Tag(0x0008, 0x0060))) continue;

        std::string modality = GetTagValue(
            reader.GetFile().GetDataSet(), 0x0008, 0x0060);

        if (modality == "RTSTRUCT") {
            qDebug() << "[RtStructReader] Found RS file:" << path.c_str();
            return path;
        }
    }

    qDebug() << "[RtStructReader] No RTSTRUCT file found in:" << dicomFolder.c_str();
    return "";
}
