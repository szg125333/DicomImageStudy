#include "RtStructReader.h"

// ── DCMTK 头文件 ──────────────────────────────────────────────
#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmdata/dctk.h>          // DcmFileFormat, DcmDataset, DcmSequenceOfItems...
#include <dcmtk/dcmdata/dcuid.h>         // UID_RTStructureSetStorage
#include <dcmtk/ofstd/ofstring.h>        // OFString

#include <sstream>
#include <QDebug>

// ============================================================
//  颜色解析："255\\0\\0" → {1.0, 0.0, 0.0}
// ============================================================

std::array<double, 3> RtStructReader::ParseColor(const std::string& s)
{
    std::array<double, 3> c = { 1.0, 0.0, 0.0 };
    if (s.empty()) return c;
    std::istringstream ss(s);
    std::string t; int i = 0;
    while (std::getline(ss, t, '\\') && i < 3) {
        while (!t.empty() && (t.front() == ' ' || t.front() == '\0')) t.erase(t.begin());
        while (!t.empty() && (t.back() == ' ' || t.back() == '\0')) t.pop_back();
        try { c[i++] = std::stod(t) / 255.0; }
        catch (...) {}
    }
    return c;
}

// ============================================================
//  轮廓点解析："x1\\y1\\z1\\x2\\y2\\z2\\..."
// ============================================================

std::vector<std::array<double, 3>> RtStructReader::ParseContourData(
    const std::string& s)
{
    std::vector<std::array<double, 3>> pts;
    if (s.empty()) return pts;
    std::istringstream ss(s);
    std::string t;
    std::vector<double> vals;
    while (std::getline(ss, t, '\\')) {
        while (!t.empty() && (t.front() == ' ' || t.front() == '\0')) t.erase(t.begin());
        while (!t.empty() && (t.back() == ' ' || t.back() == '\0')) t.pop_back();
        if (t.empty()) continue;
        try { vals.push_back(std::stod(t)); }
        catch (...) {}
    }
    for (size_t i = 0; i + 2 < vals.size(); i += 3)
        pts.push_back({ vals[i], vals[i + 1], vals[i + 2] });
    return pts;
}

// ============================================================
//  主读取函数
//
//  DCMTK Tag 常量对照：
//    DCM_StructureSetROISequence      = (3006,0020)
//    DCM_ROINumber                    = (3006,0022)
//    DCM_ROIName                      = (3006,0026)
//    DCM_ROIContourSequence           = (3006,0039)
//    DCM_ReferencedROINumber          = (3006,0084)
//    DCM_ROIDisplayColor              = (3006,002A)
//    DCM_ContourSequence              = (3006,0040)
//    DCM_ContourData                  = (3006,0050)
// ============================================================

std::vector<RtRoi> RtStructReader::Read(const std::string& filePath)
{
    std::vector<RtRoi> result;

    // ── 1. 加载文件 ───────────────────────────────────────────
    DcmFileFormat ff;
    OFCondition status = ff.loadFile(filePath.c_str());
    if (status.bad()) {
        qWarning() << "[RtStructReader] DCMTK 无法加载文件："
            << QString::fromStdString(filePath)
            << status.text();
        return result;
    }

    DcmDataset* ds = ff.getDataset();
    if (!ds) {
        qWarning() << "[RtStructReader] DcmDataset 为空";
        return result;
    }

    // 打印 Modality 做验证
    OFString modality;
    if (ds->findAndGetOFString(DCM_Modality, modality).good()) {
        qDebug() << "[RtStructReader] Modality =" << modality.c_str();
    }

    // ── 2. Structure Set ROI Sequence (3006,0020) ─────────────
    std::map<int, RtRoi> roiMap;

    DcmSequenceOfItems* roiSeq = nullptr;
    if (ds->findAndGetSequence(DCM_StructureSetROISequence, roiSeq).bad()
        || !roiSeq) {
        qWarning() << "[RtStructReader] 未找到 StructureSetROISequence (3006,0020)";
        return result;
    }

    qDebug() << "[RtStructReader] ROI Sequence 条目数：" << roiSeq->card();

    for (unsigned long i = 0; i < roiSeq->card(); ++i) {
        DcmItem* item = roiSeq->getItem(i);
        if (!item) continue;

        OFString numStr, nameStr;
        item->findAndGetOFString(DCM_ROINumber, numStr);
        item->findAndGetOFString(DCM_ROIName, nameStr);

        qDebug() << "  ROI" << i
            << "Number='" << numStr.c_str()
            << "' Name='" << nameStr.c_str() << "'";

        int roiNum = -1;
        try { roiNum = std::stoi(numStr.c_str()); }
        catch (...) { qWarning() << "  ROI Number 解析失败"; continue; }

        RtRoi roi;
        roi.roiNumber = roiNum;
        roi.roiName = nameStr.c_str();
        roiMap[roiNum] = roi;
    }

    qDebug() << "[RtStructReader] 解析到" << roiMap.size() << "个 ROI 定义";

    // ── 3. ROI Contour Sequence (3006,0039) ───────────────────
    DcmSequenceOfItems* rcSeq = nullptr;
    if (ds->findAndGetSequence(DCM_ROIContourSequence, rcSeq).bad()
        || !rcSeq) {
        qWarning() << "[RtStructReader] 未找到 ROIContourSequence (3006,0039)";
        return result;
    }

    qDebug() << "[RtStructReader] ROI Contour Sequence 条目数：" << rcSeq->card();

    for (unsigned long i = 0; i < rcSeq->card(); ++i) {
        DcmItem* rcItem = rcSeq->getItem(i);
        if (!rcItem) continue;

        // Referenced ROI Number (3006,0084)
        OFString refNumStr;
        rcItem->findAndGetOFString(DCM_ReferencedROINumber, refNumStr);

        int refNum = -1;
        try { refNum = std::stoi(refNumStr.c_str()); }
        catch (...) { qWarning() << "  RefROINumber 解析失败"; continue; }

        auto it = roiMap.find(refNum);
        if (it == roiMap.end()) {
            qWarning() << "  未找到对应 ROI =" << refNum;
            continue;
        }

        // ROI Display Color (3006,002A)
        OFString colorStr;
        if (rcItem->findAndGetOFString(DCM_ROIDisplayColor, colorStr).good()) {
            it->second.color = ParseColor(colorStr.c_str());
        }

        // Contour Sequence (3006,0040)
        DcmSequenceOfItems* cSeq = nullptr;
        if (rcItem->findAndGetSequence(DCM_ContourSequence, cSeq).bad()
            || !cSeq) {
            qWarning() << "  ROI" << refNum << "无 ContourSequence";
            continue;
        }

        qDebug() << "  ROI" << refNum << "轮廓条目：" << cSeq->card();

        for (unsigned long j = 0; j < cSeq->card(); ++j) {
            DcmItem* cItem = cSeq->getItem(j);
            if (!cItem) continue;

            // Contour Data (3006,0050)
            // 注意：ContourData 是 DS 类型，可能有多个值
            // 用 findAndGetOFStringArray 一次读取所有值拼成字符串
            OFString dataStr;
            DcmElement* dataElem = nullptr;
            if (cItem->findAndGetElement(DCM_ContourData, dataElem).bad()
                || !dataElem) {
                qWarning() << "    轮廓" << j << "无 ContourData";
                continue;
            }

            // DS 类型：用 getOFStringArray 读取所有值（DCMTK 用 '\\' 连接）
            dataElem->getOFStringArray(dataStr);
            if (dataStr.empty()) {
                qWarning() << "    轮廓" << j << "ContourData 为空";
                continue;
            }

            auto pts = ParseContourData(dataStr.c_str());
            if (pts.empty()) continue;

            RtContour contour;
            contour.points = std::move(pts);
            contour.sliceZ = contour.points.front()[2];
            it->second.contours.push_back(std::move(contour));
        }
    }

    // ── 4. 过滤空 ROI，输出结果 ──────────────────────────────
    for (auto& [num, roi] : roiMap) {
        if (!roi.contours.empty())
            result.push_back(std::move(roi));
    }

    qDebug() << "[RtStructReader] ===== 读取完成 =====";
    qDebug() << "[RtStructReader] 有效 ROI 数：" << result.size();
    for (const auto& roi : result) {
        qDebug() << "  ROI" << roi.roiNumber
            << QString::fromStdString(roi.roiName)
            << "轮廓数：" << roi.contours.size();
    }

    return result;
}
