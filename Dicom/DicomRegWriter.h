#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include <iostream>

class DicomRegWriter {
public:
    // 构造函数：传入矩阵和FrameOfReferenceUID
    DicomRegWriter(const double matrix[16], const std::string& frameUID)
        : frameOfReferenceUID(frameUID) {
        for (int i = 0; i < 16; ++i) {
            regMatrix[i] = matrix[i];
        }
    }

    // 保存为 DICOM REG 文件
    bool save(const std::string& filename) {
        DcmFileFormat fileformat;
        DcmDataset* dataset = fileformat.getDataset();

        // 基本信息
        dataset->putAndInsertString(DCM_Modality, "REG");
        dataset->putAndInsertString(DCM_SOPClassUID, UID_SpatialRegistrationStorage);
        dataset->putAndInsertString(DCM_SOPInstanceUID, dcmGenerateUniqueIdentifier(NULL, SITE_INSTANCE_UID_ROOT));
        dataset->putAndInsertString(DCM_StudyInstanceUID, dcmGenerateUniqueIdentifier(NULL, SITE_STUDY_UID_ROOT));
        dataset->putAndInsertString(DCM_SeriesInstanceUID, dcmGenerateUniqueIdentifier(NULL, SITE_SERIES_UID_ROOT));

        dataset->putAndInsertString(DCM_FrameOfReferenceUID, frameOfReferenceUID.c_str());

        // 创建 Registration Sequence
        DcmItem* regItem = new DcmItem();
        DcmItem* matrixItem = new DcmItem();

        // 写入矩阵到 Matrix Registration Sequence (0070,0309)
        matrixItem->putAndInsertFloat64Array(DcmTag(0x0070, 0x030A), regMatrix, 16);
        regItem->insertSequenceItem(DcmTag(0x0070, 0x0309), matrixItem);
        dataset->insertSequenceItem(DcmTag(0x0070, 0x0308), regItem);

        // 保存文件
        OFCondition status = fileformat.saveFile(filename.c_str(), EXS_LittleEndianExplicit);
        if (status.good()) {
            std::cout << "成功保存 REG 文件: " << filename << std::endl;
            return true;
        }
        else {
            std::cerr << "保存失败: " << status.text() << std::endl;
            return false;
        }
    }

private:
    double regMatrix[16];
    std::string frameOfReferenceUID;
};
