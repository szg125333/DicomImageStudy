#include "RtStructReader.h"

#include <gdcmReader.h>
#include <gdcmDataSet.h>
#include <gdcmSequenceOfItems.h>
#include <gdcmAttribute.h>
#include <gdcmSmartPointer.h>
#include <gdcmMediaStorage.h>

#include <iostream>
#include <sstream>

// ============================================================
//  辅助：从字符串 "r\\g\\b" 解析 RGB 颜色（0~255 → 0~1）
// ============================================================
static std::array<double, 3> ParseColor(const std::string& colorStr) {
    std::array<double, 3> color = { 1.0, 0.0, 0.0 }; // 默认红色
    std::istringstream iss(colorStr);
    std::string token;
    int idx = 0;
    while (std::getline(iss, token, '\\') && idx < 3) {
        try {
            color[idx] = std::stod(token) / 255.0;
        }
        catch (...) {}
        idx++;
    }
    return color;
}

std::shared_ptr<RTStructureData> RtStructReader::Read(const std::string& filename)
{
    // 1. 读取 DICOM 文件
    gdcm::Reader reader;
    reader.SetFileName(filename.c_str());
    if (!reader.Read()) {
        std::cerr << "[RtStructReader] Failed to read file: " << filename << std::endl;
        return nullptr;
    }

    const gdcm::DataSet& ds = reader.GetFile().GetDataSet();

    // 2. 验证必要的 Sequence 是否存在
    // (3006,0020) Structure Set ROI Sequence
    gdcm::Tag tagSSROISeq(0x3006, 0x0020);
    // (3006,0039) ROI Contour Sequence
    gdcm::Tag tagROIContourSeq(0x3006, 0x0039);

    if (!ds.FindDataElement(tagSSROISeq)) {
        std::cerr << "[RtStructReader] Missing (3006,0020) Structure Set ROI Sequence" << std::endl;
        return nullptr;
    }
    if (!ds.FindDataElement(tagROIContourSeq)) {
        std::cerr << "[RtStructReader] Missing (3006,0039) ROI Contour Sequence" << std::endl;
        return nullptr;
    }

    auto result = std::make_shared<RTStructureData>();

    // 读取全局标签
    if (ds.FindDataElement(gdcm::Tag(0x3006, 0x0002))) {
        gdcm::Attribute<0x3006, 0x02> label;
        label.SetFromDataSet(ds);
        result->label = label.GetValue();
    }
    if (ds.FindDataElement(gdcm::Tag(0x3006, 0x0004))) {
        gdcm::Attribute<0x3006, 0x04> atname;
        atname.SetFromDataSet(ds);
        result->name = atname.GetValue();
    }

    // ============================================================
    //  3. 解析 Structure Set ROI Sequence → ROI 名称和编号
    // ============================================================
    const gdcm::DataElement& ssroiDE = ds.GetDataElement(tagSSROISeq);
    gdcm::SmartPointer<gdcm::SequenceOfItems> ssroiSQ = ssroiDE.GetValueAsSQ();
    if (!ssroiSQ || ssroiSQ->GetNumberOfItems() == 0) {
        std::cerr << "[RtStructReader] Empty Structure Set ROI Sequence" << std::endl;
        return nullptr;
    }

    // 建立 ROI Number → ROI Name 映射
    for (unsigned int i = 0; i < ssroiSQ->GetNumberOfItems(); ++i) {
        const gdcm::Item& item = ssroiSQ->GetItem(i + 1); // GDCM Item 从 1 开始
        const gdcm::DataSet& nestedDS = item.GetNestedDataSet();

        // ROI Number (3006,0022)
        gdcm::Attribute<0x3006, 0x0022> roiNumAttr;
        roiNumAttr.SetFromDataSet(nestedDS);
        int roiNum = roiNumAttr.GetValue();

        // ROI Name (3006,0026)
        gdcm::Attribute<0x3006, 0x0026> roiNameAttr;
        roiNameAttr.SetFromDataSet(nestedDS);
        std::string roiName = roiNameAttr.GetValue();

        RTStructureROI roi;
        roi.roiNumber = roiNum;
        roi.name = roiName;
        result->rois[roiNum] = roi;
    }

    // ============================================================
    //  4. 解析 ROI Contour Sequence → 轮廓数据和颜色
    // ============================================================
    const gdcm::DataElement& roiContourDE = ds.GetDataElement(tagROIContourSeq);
    gdcm::SmartPointer<gdcm::SequenceOfItems> roiContourSQ = roiContourDE.GetValueAsSQ();
    if (!roiContourSQ || roiContourSQ->GetNumberOfItems() == 0) {
        std::cerr << "[RtStructReader] Empty ROI Contour Sequence" << std::endl;
        return result; // 有 ROI 信息但没有轮廓也可以返回
    }

    for (unsigned int i = 0; i < roiContourSQ->GetNumberOfItems(); ++i) {
        const gdcm::Item& item = roiContourSQ->GetItem(i + 1);
        const gdcm::DataSet& nestedDS = item.GetNestedDataSet();

        // Referenced ROI Number (3006,0084)
        gdcm::Attribute<0x3006, 0x0084> refROINum;
        refROINum.SetFromDataSet(nestedDS);
        int roiNum = refROINum.GetValue();

        // 确保该 ROI 在 map 中
        if (result->rois.find(roiNum) == result->rois.end()) {
            continue;
        }

        RTStructureROI& roi = result->rois[roiNum];

        // ROI Display Color (3006,002A) — 可选
        gdcm::Tag tagColor(0x3006, 0x002A);
        if (nestedDS.FindDataElement(tagColor)) {
            const gdcm::DataElement& colorDE = nestedDS.GetDataElement(tagColor);
            if (colorDE.GetByteValue()) {
                std::string colorStr(colorDE.GetByteValue()->GetPointer(),
                    colorDE.GetByteValue()->GetLength());
                roi.color = ParseColor(colorStr);
            }
        }

        // Contour Sequence (3006,0040) — 包含各切片的轮廓
        gdcm::Tag tagContourSeq(0x3006, 0x0040);
        if (!nestedDS.FindDataElement(tagContourSeq)) {
            continue; // 该 ROI 没有轮廓数据
        }

        const gdcm::DataElement& contourSeqDE = nestedDS.GetDataElement(tagContourSeq);
        gdcm::SmartPointer<gdcm::SequenceOfItems> contourSQ = contourSeqDE.GetValueAsSQ();
        if (!contourSQ || contourSQ->GetNumberOfItems() == 0) {
            continue;
        }

        // 遍历每一条轮廓
        for (unsigned int c = 0; c < contourSQ->GetNumberOfItems(); ++c) {
            const gdcm::Item& contourItem = contourSQ->GetItem(c + 1);
            const gdcm::DataSet& contourDS = contourItem.GetNestedDataSet();

            // Contour Geometric Type (3006,0042)
            gdcm::Tag tagGeomType(0x3006, 0x0042);
            if (contourDS.FindDataElement(tagGeomType)) {
                const gdcm::DataElement& geomDE = contourDS.GetDataElement(tagGeomType);
                if (geomDE.GetByteValue()) {
                    std::string geomType(geomDE.GetByteValue()->GetPointer(),
                        geomDE.GetByteValue()->GetLength());
                    // 去掉末尾空格
                    while (!geomType.empty() && geomType.back() == ' ')
                        geomType.pop_back();
                    // 只处理 CLOSED_PLANAR
                    if (geomType != "CLOSED_PLANAR") {
                        continue;
                    }
                }
            }

            // Number of Contour Points (3006,0046)
            gdcm::Attribute<0x3006, 0x0046> numPtsAttr;
            numPtsAttr.SetFromDataSet(contourDS);
            int numPts = numPtsAttr.GetValue();

            if (numPts <= 0) continue;

            // Contour Data (3006,0050) — 核心坐标数据
            gdcm::Tag tagContourData(0x3006, 0x0050);
            if (!contourDS.FindDataElement(tagContourData)) continue;

            const gdcm::DataElement& contourDataDE = contourDS.GetDataElement(tagContourData);
            gdcm::Attribute<0x3006, 0x0050> contourDataAttr;
            contourDataAttr.SetFromDataElement(contourDataDE);
            const double* values = contourDataAttr.GetValues();
            unsigned int numValues = contourDataAttr.GetNumberOfValues();

            if (numValues < static_cast<unsigned int>(numPts) * 3) continue;

            RTContour contour;
            contour.points.resize(numPts);

            for (int p = 0; p < numPts; ++p) {
                contour.points[p][0] = values[p * 3 + 0]; // X
                contour.points[p][1] = values[p * 3 + 1]; // Y
                contour.points[p][2] = values[p * 3 + 2]; // Z
            }

            // Z 坐标取第一个点的 Z
            if (numPts > 0) {
                contour.z = contour.points[0][2];
            }

            roi.contours.push_back(std::move(contour));
        }

        std::cout << "[RtStructReader] ROI \"" << roi.name
            << "\" (num=" << roiNum << "): "
            << roi.contours.size() << " contours loaded" << std::endl;
    }

    return result;
}