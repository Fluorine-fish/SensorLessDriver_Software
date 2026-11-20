/**
*   @file FocAlg.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/10/29
*   @version 1.0
*   @note
*/
#include "FocAlg.h"
#include "BspFlash.h"
#include "math.h"
#include "string.h"

static const uint32_t FocAlg_Crc32Table[256] = {
    0x00000000u, 0x77073096u, 0xEE0E612Cu, 0x990951BAu,
    0x076DC419u, 0x706AF48Fu, 0xE963A535u, 0x9E6495A3u,
    0x0EDB8832u, 0x79DCB8A4u, 0xE0D5E91Eu, 0x97D2D988u,
    0x09B64C2Bu, 0x7EB17CBDu, 0xE7B82D07u, 0x90BF1D91u,
    0x1DB71064u, 0x6AB020F2u, 0xF3B97148u, 0x84BE41DEu,
    0x1ADAD47Du, 0x6DDDE4EBu, 0xF4D4B551u, 0x83D385C7u,
    0x136C9856u, 0x646BA8C0u, 0xFD62F97Au, 0x8A65C9ECu,
    0x14015C4Fu, 0x63066CD9u, 0xFA0F3D63u, 0x8D080DF5u,
    0x3B6E20C8u, 0x4C69105Eu, 0xD56041E4u, 0xA2677172u,
    0x3C03E4D1u, 0x4B04D447u, 0xD20D85FDu, 0xA50AB56Bu,
    0x35B5A8FAu, 0x42B2986Cu, 0xDBBBC9D6u, 0xACBCF940u,
    0x32D86CE3u, 0x45DF5C75u, 0xDCD60DCFu, 0xABD13D59u,
    0x26D930ACu, 0x51DE003Au, 0xC8D75180u, 0xBFD06116u,
    0x21B4F4B5u, 0x56B3C423u, 0xCFBA9599u, 0xB8BDA50Fu,
    0x2802B89Eu, 0x5F058808u, 0xC60CD9B2u, 0xB10BE924u,
    0x2F6F7C87u, 0x58684C11u, 0xC1611DABu, 0xB6662D3Du,
    0x76DC4190u, 0x01DB7106u, 0x98D220BCu, 0xEFD5102Au,
    0x71B18589u, 0x06B6B51Fu, 0x9FBFE4A5u, 0xE8B8D433u,
    0x7807C9A2u, 0x0F00F934u, 0x9609A88Eu, 0xE10E9818u,
    0x7F6A0DBBu, 0x086D3D2Du, 0x91646C97u, 0xE6635C01u,
    0x6B6B51F4u, 0x1C6C6162u, 0x856530D8u, 0xF262004Eu,
    0x6C0695EDu, 0x1B01A57Bu, 0x8208F4C1u, 0xF50FC457u,
    0x65B0D9C6u, 0x12B7E950u, 0x8BBEB8EAu, 0xFCB9887Cu,
    0x62DD1DDFu, 0x15DA2D49u, 0x8CD37CF3u, 0xFBD44C65u,
    0x4DB26158u, 0x3AB551CEu, 0xA3BC0074u, 0xD4BB30E2u,
    0x4ADFA541u, 0x3DD895D7u, 0xA4D1C46Du, 0xD3D6F4FBu,
    0x4369E96Au, 0x346ED9FCu, 0xAD678846u, 0xDA60B8D0u,
    0x44042D73u, 0x33031DE5u, 0xAA0A4C5Fu, 0xDD0D7CC9u,
    0x5005713Cu, 0x270241AAu, 0xBE0B1010u, 0xC90C2086u,
    0x5768B525u, 0x206F85B3u, 0xB966D409u, 0xCE61E49Fu,
    0x5EDEF90Eu, 0x29D9C998u, 0xB0D09822u, 0xC7D7A8B4u,
    0x59B33D17u, 0x2EB40D81u, 0xB7BD5C3Bu, 0xC0BA6CADu,
    0xEDB88320u, 0x9ABFB3B6u, 0x03B6E20Cu, 0x74B1D29Au,
    0xEAD54739u, 0x9DD277AFu, 0x04DB2615u, 0x73DC1683u,
    0xE3630B12u, 0x94643B84u, 0x0D6D6A3Eu, 0x7A6A5AA8u,
    0xE40ECF0Bu, 0x9309FF9Du, 0x0A00AE27u, 0x7D079EB1u,
    0xF00F9344u, 0x8708A3D2u, 0x1E01F268u, 0x6906C2FEu,
    0xF762575Du, 0x806567CBu, 0x196C3671u, 0x6E6B06E7u,
    0xFED41B76u, 0x89D32BE0u, 0x10DA7A5Au, 0x67DD4ACCu,
    0xF9B9DF6Fu, 0x8EBEEFF9u, 0x17B7BE43u, 0x60B08ED5u,
    0xD6D6A3E8u, 0xA1D1937Eu, 0x38D8C2C4u, 0x4FDFF252u,
    0xD1BB67F1u, 0xA6BC5767u, 0x3FB506DDu, 0x48B2364Bu,
    0xD80D2BDAu, 0xAF0A1B4Cu, 0x36034AF6u, 0x41047A60u,
    0xDF60EFC3u, 0xA867DF55u, 0x316E8EEFu, 0x4669BE79u,
    0xCB61B38Cu, 0xBC66831Au, 0x256FD2A0u, 0x5268E236u,
    0xCC0C7795u, 0xBB0B4703u, 0x220216B9u, 0x5505262Fu,
    0xC5BA3BBEu, 0xB2BD0B28u, 0x2BB45A92u, 0x5CB36A04u,
    0xC2D7FFA7u, 0xB5D0CF31u, 0x2CD99E8Bu, 0x5BDEAE1Du,
    0x9B64C2B0u, 0xEC63F226u, 0x756AA39Cu, 0x026D930Au,
    0x9C0906A9u, 0xEB0E363Fu, 0x72076785u, 0x05005713u,
    0x95BF4A82u, 0xE2B87A14u, 0x7BB12BAEu, 0x0CB61B38u,
    0x92D28E9Bu, 0xE5D5BE0Du, 0x7CDCEFB7u, 0x0BDBDF21u,
    0x86D3D2D4u, 0xF1D4E242u, 0x68DDB3F8u, 0x1FDA836Eu,
    0x81BE16CDu, 0xF6B9265Bu, 0x6FB077E1u, 0x18B74777u,
    0x88085AE6u, 0xFF0F6A70u, 0x66063BCAu, 0x11010B5Cu,
    0x8F659EFFu, 0xF862AE69u, 0x616BFFD3u, 0x166CCF45u,
    0xA00AE278u, 0xD70DD2EEu, 0x4E048354u, 0x3903B3C2u,
    0xA7672661u, 0xD06016F7u, 0x4969474Du, 0x3E6E77DBu,
    0xAED16A4Au, 0xD9D65ADCu, 0x40DF0B66u, 0x37D83BF0u,
    0xA9BCAE53u, 0xDEBB9EC5u, 0x47B2CF7Fu, 0x30B5FFE9u,
    0xBDBDF21Cu, 0xCABAC28Au, 0x53B39330u, 0x24B4A3A6u,
    0xBAD03605u, 0xCDD70693u, 0x54DE5729u, 0x23D967BFu,
    0xB3667A2Eu, 0xC4614AB8u, 0x5D681B02u, 0x2A6F2B94u,
    0xB40BBE37u, 0xC30C8EA1u, 0x5A05DF1Bu, 0x2D02EF8Du,
};

uint32_t FocAlg_Crc32_Calc(const void* data, uint32_t length) {
    if (data == NULL) return 0u;

    const uint8_t* input = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFFu;

    while (length-- > 0u) {
        uint8_t table_index = (uint8_t)(crc ^ *input++);
        crc = (crc >> 8) ^ FocAlg_Crc32Table[table_index];
    }

    return crc ^ 0xFFFFFFFFu;
}

//Encoder
static void FocAlg_HallEncoder_Reset(FocAlg_HallEncoder_s* encoder) {
    encoder->calib_cnt = 0;
    memset(encoder->matrix_P, 0, sizeof(encoder->matrix_P));
    memset(encoder->matrix_R, 0, sizeof(encoder->matrix_R));

    encoder->m11 = 0;
    encoder->m12 = 0;
    encoder->m21 = 0;
    encoder->m22 = 0;
    encoder->o1 = 0;
    encoder->o2 = 0;

    encoder->sin = 0;
    encoder->cos = 0;

    encoder->is_calibrated = false;
}

void FocAlg_HallEncoder_Init(FocAlg_HallEncoder_s* encoder,
    float kp, float ki, float ka, float ts, float out_max, float error_sum_max) {
    encoder->error_code = 0;

    FocAlg_HallEncoder_Reset(encoder);
    memset(&encoder->flash, 0, sizeof(encoder->flash));
    FocAlg_SinCosPll_Init(&encoder->pll, kp, ki, ka, ts, out_max, error_sum_max);
}

void FocAlg_HallEncoder_Calib(FocAlg_HallEncoder_s* encoder, uint16_t *input, float theta_ref) {
    if (encoder == NULL) return;
    if (encoder->is_calibrated) return;

#ifndef ENCODER_NEED_CALIB
    BspFlash_Read(ADDR_FLASH_SECTOR_11 - ADDR_FLASH_SECTOR_0, &encoder->flash, sizeof(encoder->flash));
    uint32_t crc32 = FocAlg_Crc32_Calc(&encoder->flash, sizeof(encoder->flash) - sizeof(encoder->flash.crc32));
    if (crc32 == encoder->flash.crc32 && encoder->flash.magic == ENCODER_HALL_MAGIC && encoder->flash.version == ENCODER_HALL_VERSION) {
        encoder->m11 = encoder->flash.m11;
        encoder->m12 = encoder->flash.m12;
        encoder->m21 = encoder->flash.m21;
        encoder->m22 = encoder->flash.m22;
        encoder->o1 = encoder->flash.o1;
        encoder->o2 = encoder->flash.o2;

        encoder->is_calibrated = true;
        return;
    }
#endif

    static uint16_t hall_minmax[2][2] = {}; // 内数组存放minmax，两个hall分开存放
    static uint16_t adc_out_range_cnt[2] = {}; // 统计ADC超出边界计数

    if (encoder->calib_cnt < ENCODER_CALIB_CNT + ENCODER_BLOCK_TEST_CNT) {
        float sin = FocMath_FastSin(theta_ref);
        float cos = FocMath_FastCos(theta_ref);

        if (encoder->calib_cnt < ENCODER_BLOCK_TEST_CNT) { // 检测是否发生堵转
            if (encoder->calib_cnt == 0) {
                hall_minmax[0][0] = input[0];
                hall_minmax[0][1] = input[0];
                hall_minmax[1][0] = input[1];
                hall_minmax[1][1] = input[1];
            }else if (encoder->calib_cnt == ENCODER_BLOCK_TEST_CNT - 1) {
                if (FOCMATH_ABS(hall_minmax[0][1] - hall_minmax[0][0]) < 400 ||
                    FOCMATH_ABS(hall_minmax[1][1] - hall_minmax[1][0]) < 400) {
                    encoder->error_code |= (uint8_t)ENCODER_HALL_ERROR;
                    encoder->error_code |= (uint8_t)ENCODER_MOTOR_BLOCKED_ERROR;

                    FocAlg_HallEncoder_Reset(encoder);
                    return;
                }
            }else {
                // 迭代最大最小值
                if (input[0] < hall_minmax[0][0]) hall_minmax[0][0] = input[0];
                if (input[0] > hall_minmax[0][1]) hall_minmax[0][1] = input[0];
                if (input[1] < hall_minmax[1][0]) hall_minmax[1][0] = input[1];
                if (input[1] > hall_minmax[1][1]) hall_minmax[1][1] = input[1];
            }

        }else { // 若没有堵转就正常积累数据
            // 统计接近ADC边界的异常数据
            if (input[0] < 200 || input[0] > 3896) adc_out_range_cnt[0] += 1;
            if (input[1] < 200 || input[1] > 3896) adc_out_range_cnt[1] += 1;

            encoder->matrix_P[0][0] += sin * (float)input[0];
            encoder->matrix_P[0][1] += sin * (float)input[1];
            encoder->matrix_P[1][0] += cos * (float)input[0];
            encoder->matrix_P[1][1] += cos * (float)input[1];
            encoder->matrix_P[2][0] += (float)input[0];
            encoder->matrix_P[2][1] += (float)input[1];

            encoder->matrix_R[0][0] += sin * sin;
            encoder->matrix_R[0][1] += sin * cos;
            encoder->matrix_R[0][2] += sin;
            encoder->matrix_R[1][0] += sin * cos;
            encoder->matrix_R[1][1] += cos * cos;
            encoder->matrix_R[1][2] += cos;
            encoder->matrix_R[2][0] += sin;
            encoder->matrix_R[2][1] += cos;
            encoder->matrix_R[2][2] += 1;
        }

        encoder->calib_cnt ++;
    }else {
        if ((float)adc_out_range_cnt[0] > ENCODER_CALIB_CNT * 0.1f ||
            (float)adc_out_range_cnt[1] > ENCODER_CALIB_CNT * 0.1f) {
            encoder->error_code |= (uint8_t)ENCODER_ADC_ERROR;

            FocAlg_HallEncoder_Reset(encoder);
            return;
        }

        encoder->matrix_P[0][0] /= (float)encoder->calib_cnt;
        encoder->matrix_P[0][1] /= (float)encoder->calib_cnt;
        encoder->matrix_P[1][0] /= (float)encoder->calib_cnt;
        encoder->matrix_P[1][1] /= (float)encoder->calib_cnt;
        encoder->matrix_P[2][0] /= (float)encoder->calib_cnt;
        encoder->matrix_P[2][1] /= (float)encoder->calib_cnt;

        encoder->matrix_R[0][0] /= (float)encoder->calib_cnt;
        encoder->matrix_R[0][1] /= (float)encoder->calib_cnt;
        encoder->matrix_R[0][2] /= (float)encoder->calib_cnt;
        encoder->matrix_R[1][0] /= (float)encoder->calib_cnt;
        encoder->matrix_R[1][1] /= (float)encoder->calib_cnt;
        encoder->matrix_R[1][2] /= (float)encoder->calib_cnt;
        encoder->matrix_R[2][0] /= (float)encoder->calib_cnt;
        encoder->matrix_R[2][1] /= (float)encoder->calib_cnt;
        encoder->matrix_R[2][2] /= (float)encoder->calib_cnt;

        // 1 Cholesky分解
        float l00 = FocMath_FastSqrt(encoder->matrix_R[0][0]);
        if (l00 <= 0 || l00 * l00 / encoder->matrix_R[0][0] < 1e-4) { // 数据失效
            encoder->error_code |= (uint8_t)ENCODER_CHOLESKY_ERROR;

            FocAlg_HallEncoder_Reset(encoder);
            return;
        }
        float l10 = encoder->matrix_R[1][0] / l00;
        float l20 = encoder->matrix_R[2][0] / l00;
        float l11 = FocMath_FastSqrt(encoder->matrix_R[1][1] - l10 * l10);
        if (l11 <= 0 || l11 * l11 / encoder->matrix_R[1][1] < 1e-4) { // 数据失效
            encoder->error_code |= (uint8_t)ENCODER_CHOLESKY_ERROR;

            FocAlg_HallEncoder_Reset(encoder);
            return;
        }
        float l21 = (encoder->matrix_R[2][1] - l10 * l20) / l11;
        float l22 = FocMath_FastSqrt(encoder->matrix_R[2][2] - l20 * l20 - l21 * l21);
        if (l22 <= 0 || l22 * l22 / encoder->matrix_R[2][2] < 1e-4) { // 数据失效
            encoder->error_code |= (uint8_t)ENCODER_CHOLESKY_ERROR;

            FocAlg_HallEncoder_Reset(encoder);
            return;
        }

        // 2 过渡矩阵Z LZ=P
        float z00 = encoder->matrix_P[0][0] / l00;
        float z01 = encoder->matrix_P[0][1] / l00;
        float z10 = (encoder->matrix_P[1][0] - l10 * z00) / l11;
        float z11 = (encoder->matrix_P[1][1] - l10 * z01) / l11;
        float z20 = (encoder->matrix_P[2][0] - l20 * z00 - l21 * z10) / l22;
        float z21 = (encoder->matrix_P[2][1] - l20 * z01 - l21 * z11) / l22;

        // 3 求解W L^TW = Z
        encoder->o1 = z20 / l22;
        encoder->o2 = z21 / l22;
        float bx = (z10 - l21 * encoder->o1) / l11;
        float by = (z11 - l21 * encoder->o2) / l11;
        float ax = (z00 - l10 * bx - l20 * encoder->o1) / l00;
        float ay = (z01 - l10 * by - l20 * encoder->o2) / l00;

        // 4 求解参数
        float detA = ax * by - bx*ay;
        float scale = FOCMATH_ABS(ax * by) + FOCMATH_ABS(bx * ay);
        if (detA == 0 || FOCMATH_ABS(detA) < 1e-3 * scale) { // 数据失效
            encoder->error_code |= (uint8_t)ENCODER_DET_A_ERROR;

            FocAlg_HallEncoder_Reset(encoder);
            return;
        }
        encoder->m11 = by / detA;
        encoder->m12 = - bx / detA;
        encoder->m21 = - ay / detA;
        encoder->m22 = ax / detA;

        encoder->flash.magic = ENCODER_HALL_MAGIC;
        encoder->flash.version = ENCODER_HALL_VERSION;
        encoder->flash.size = sizeof(FocAlg_HallEncoder_Flash_s);
        encoder->flash.m11 = encoder->m11;
        encoder->flash.m12 = encoder->m12;
        encoder->flash.m21 = encoder->m21;
        encoder->flash.m22 = encoder->m22;
        encoder->flash.o1 = encoder->o1;
        encoder->flash.o2 = encoder->o2;
        encoder->flash.crc32 = FocAlg_Crc32_Calc(&encoder->flash, sizeof(encoder->flash) - sizeof(encoder->flash.crc32));

        if (!BspFlash_Write(FLASH_Sector_11, (uint32_t*)&encoder->flash, sizeof(encoder->flash))) {
            encoder->error_code |= (uint8_t)ENCODER_WRITE_FLASH_ERROR;

            FocAlg_HallEncoder_Reset(encoder);
            return;
        };

        encoder->is_calibrated = true;
    }
}

void FocAlg_HallEncoder_Calc(FocAlg_HallEncoder_s* encoder, uint16_t *input) {
    if (encoder == NULL) return;
    if (!encoder->is_calibrated) return;

    float u1 = (float)input[0] - encoder->o1;
    float u2 = (float)input[1] - encoder->o2;

    float sin_raw = encoder->m11 * u1 + encoder->m12 * u2;
    float cos_raw = encoder->m21 * u1 + encoder->m22 * u2;

    float comp_sin = FocMath_FastSin(ENCODER_COMP_ANGLE);
    float comp_cos = FocMath_FastCos(ENCODER_COMP_ANGLE);

    encoder->sin = sin_raw * comp_cos + cos_raw * comp_sin;
    encoder->cos = cos_raw * comp_cos - sin_raw * comp_sin;

    FocAlg_SinCosPll_Calc(&encoder->pll, encoder->sin, encoder->cos);
}

// Hfi
float FocApp_Hfi_BootChime(uint8_t *finished, float dt) {
    static const float boot_freq[3] = {
        529.84f, 595.28f, 796.82f
    };
    static const float boot_gain[3] = {
        1.00f, 1.33f, 1.45f
    };
    static const uint16_t boot_samples[3] = {
        5000U, 5000U, 8000U
    };
    static uint32_t boot_cnt = 0U;
    static uint8_t boot_index = 0U;
    static float boot_phi = 0.0f;

    if (boot_index >= 3U) {
        *finished = 1U;
        return 0.0f;
    }
    *finished = 0U;

    float waveform = 5.0f * FocMath_FastSin(boot_phi);
    if (waveform > 1.0f) waveform = 1.0f;
    else if (waveform < -1.0f) waveform = -1.0f;

    float envelope = 1.0f;
    if (boot_cnt < 60U) {
        envelope = (float)boot_cnt / 60.0f;
    } else if (boot_index == 2U
            && boot_cnt > boot_samples[2] - 100U) {
        envelope = (float)(boot_samples[2] - boot_cnt) / 100.0f;
    }

    float voltage_q = HFI_VQ * boot_gain[boot_index]
                    * envelope * waveform;

    boot_phi += DOUBLE_PI * boot_freq[boot_index] * dt;
    if (boot_phi >= DOUBLE_PI) boot_phi -= DOUBLE_PI;

    boot_cnt++;
    if (boot_cnt >= boot_samples[boot_index]) {
        boot_cnt = 0U;
        boot_index++;
    }

    return voltage_q;
}

float FocAlg_Hfi_AlarmChime(float dt) {
    static const uint16_t beep_samples = 1700U;
    static const uint16_t beep_period = 3200U;
    static const uint32_t alarm_period = 22600U;
    static uint32_t alarm_cnt = 0U;
    static float alarm_phi = 0.0f;

    uint16_t beep_pos = 0U;
    uint8_t beep_active = 0U;

    if (alarm_cnt < beep_samples) {
        beep_pos = (uint16_t)alarm_cnt;
        beep_active = 1U;
    } else if (alarm_cnt >= beep_period
            && alarm_cnt < beep_period + beep_samples) {
        beep_pos = (uint16_t)(alarm_cnt - beep_period);
        beep_active = 1U;
    } else if (alarm_cnt >= 2U * beep_period
            && alarm_cnt < 2U * beep_period + beep_samples) {
        beep_pos = (uint16_t)(alarm_cnt - 2U * beep_period);
        beep_active = 1U;
    }

    float voltage_q = 0.0f;
    if (beep_active) {
        float waveform = 5.0f * FocMath_FastSin(alarm_phi);
        if (waveform > 1.0f) waveform = 1.0f;
        else if (waveform < -1.0f) waveform = -1.0f;

        float envelope = 1.0f;
        if (beep_pos < 100U) {
            envelope = (float)beep_pos / 100.0f;
        } else if (beep_pos > beep_samples - 200U) {
            envelope = (float)(beep_samples - beep_pos) / 200.0f;
        }

        voltage_q = HFI_VQ * 1.55f * envelope * waveform;
    }

    alarm_phi += DOUBLE_PI * 1065.34f * dt;
    if (alarm_phi >= DOUBLE_PI) alarm_phi -= DOUBLE_PI;

    alarm_cnt++;
    if (alarm_cnt >= alarm_period) alarm_cnt = 0U;

    return voltage_q;
}

// 将静止两相坐标系的矢量解出分量
void FocAlg_DecoupleVoltageVector(float model, float theta, float* alphabeta) {
    alphabeta[0] = model * FocMath_FastCos(theta);
    alphabeta[1] = model * FocMath_FastSin(theta);
}

void FocAlg_ClarkeTrans(float* input, float* output) {
    if (input == NULL || output == NULL) return;

    output[0] = TWO_THREES * (input[0] - 0.5f * input[1] - 0.5f * input[2]);
    output[1] = TWO_THREES * (HALF_SQRT3 * input[1] - HALF_SQRT3 * input[2]);
}

void FocAlg_ParkTrans(float theta_e, float* input, float* output) {
    if (input == NULL || output == NULL) return;

    output[0] = input[0] * FocMath_FastCos(theta_e) + input[1] * FocMath_FastSin(theta_e);
    output[1] = - input[0] * FocMath_FastSin(theta_e) + input[1] * FocMath_FastCos(theta_e);
}

void FocAlg_DqzTrans(float theta_e, float* input, float* output) {
    if (input == NULL || output == NULL) return;

    output[0] = input[0] * FocMath_FastCos(theta_e)
        + input[1] * FocMath_FastCos(theta_e - TWO_THREES_PI)
        + input[2] * FocMath_FastCos(theta_e - FOUR_THREES_PI);

    output[1] = - input[0] * FocMath_FastSin(theta_e)
        - input[1] * FocMath_FastSin(theta_e - TWO_THREES_PI)
        - input[2] * FocMath_FastSin(theta_e - FOUR_THREES_PI);
}

void FocAlg_InvParkTrans(float theta_e, float* input, float* output) {
    if (input == NULL || output == NULL) return;

    output[0] = input[0] * FocMath_FastCos(theta_e) - input[1] * FocMath_FastSin(theta_e);
    output[1] = input[0] * FocMath_FastSin(theta_e) + input[1] * FocMath_FastCos(theta_e);
}

void FocAlg_InvClarkeTrans(float* input, float* output) {
    if (input == NULL || output == NULL) return;

    output[0] = input[0];
    output[1] = - input[0] * 0.5f + input[1] * HALF_SQRT3;
    output[2] = - input[0] * 0.5f - input[1] * HALF_SQRT3;
}

// 对dq下的电压矢量限幅，结合实际vbus的调制能力
void FocAlg_DqVoltageVectorClamp(Foc_s *foc, float* input) {
    if (input == NULL || foc == NULL) return;

    const float duty_range = (MAX_DUTY - MIN_DUTY);
    float max_model = duty_range * foc->vbus * duty_range  * foc->vbus / 3.f;

    if (input[0] * input[0] + input[1] * input[1] > max_model) {
        if (input[0] * input[0] > max_model) {
            input[0] = (input[0] > 0) ?  FocMath_FastSqrt(max_model) : - FocMath_FastSqrt(max_model);
            input[1] = 0.f;
        }else {
            input[1] = (input[1] > 0) ? FocMath_FastSqrt(max_model - input[0] * input[0]) : - FocMath_FastSqrt(max_model - input[0] * input[0]);
        }
    }else {
        return;
    }
}

// v_d = -omega_e * Lq * iq, v_q = omega_e * Ld * id
void FocAlg_DqCurrentCrossCouplingFeedforward(float omega_e, float ld, float lq,
        const float* i_dq, float* u_dq) {
    if (i_dq == NULL || u_dq == NULL) return;

    u_dq[0] -= omega_e * lq * i_dq[1];
    u_dq[1] += omega_e * ld * i_dq[0];
}

void FocAlg_VoltageCurrentConstructOpenLoop(Foc_s *foc) {
    FocAlg_ClarkeTrans(foc->phase_current, foc->fdb_i_alphabeta);
    FocAlg_ParkTrans(foc->theta_e, foc->fdb_i_alphabeta, foc->fdb_i_dq);
    FocAlg_ClarkeTrans(foc->phase_voltage, foc->fdb_u_alphabeta);
}

void FocAlg_ZeroInjectSvpwm(Foc_s *foc, float* u_alphabeta) {
    float outer_voltage[3] = {};

    FocAlg_InvClarkeTrans(u_alphabeta, outer_voltage);

    float min = outer_voltage[0];
    float max = outer_voltage[0];
    if (outer_voltage[1] > max) max = outer_voltage[1];
    if (outer_voltage[1] < min) min = outer_voltage[1];
    if (outer_voltage[2] > max) max = outer_voltage[2];
    if (outer_voltage[2] < min) min = outer_voltage[2];

    float u0 = - (max + min) / 2.f;
    foc->duty[0] = (u0 + outer_voltage[0]) / foc->vbus + 0.5f;
    foc->duty[1] = (u0 + outer_voltage[1]) / foc->vbus + 0.5f;
    foc->duty[2] = (u0 + outer_voltage[2]) / foc->vbus + 0.5f;

    if (foc->duty[0] > MAX_DUTY) foc->duty[0] = MAX_DUTY;
    else if (foc->duty[0] < MIN_DUTY) foc->duty[0] = MIN_DUTY;
    if (foc->duty[1] > MAX_DUTY) foc->duty[1] = MAX_DUTY;
    else if (foc->duty[1] < MIN_DUTY) foc->duty[1] = MIN_DUTY;
    if (foc->duty[2] > MAX_DUTY) foc->duty[2] = MAX_DUTY;
    else if (foc->duty[2] < MIN_DUTY) foc->duty[2] = MIN_DUTY;
}

void FocAlg_Spwm(Foc_s *foc, float* u_alphabeta) {
    float outer_voltage[3] = {};

    FocAlg_InvClarkeTrans(u_alphabeta, outer_voltage);

    foc->duty[0] = (outer_voltage[0]) / foc->vbus + 0.5f;
    foc->duty[1] = (outer_voltage[1]) / foc->vbus + 0.5f;
    foc->duty[2] = (outer_voltage[2]) / foc->vbus + 0.5f;

    if (foc->duty[0] > MAX_DUTY) foc->duty[0] = MAX_DUTY;
    else if (foc->duty[0] < MIN_DUTY) foc->duty[0] = MIN_DUTY;
    if (foc->duty[1] > MAX_DUTY) foc->duty[1] = MAX_DUTY;
    else if (foc->duty[1] < MIN_DUTY) foc->duty[1] = MIN_DUTY;
    if (foc->duty[2] > MAX_DUTY) foc->duty[2] = MAX_DUTY;
    else if (foc->duty[2] < MIN_DUTY) foc->duty[2] = MIN_DUTY;
}

void FocAlg_BackCalcPIController_Init(FocAlg_Pid_s *pid,
        float kp, float ki, float ka, float dt, float out_max, float error_sum_max) {
    pid->kp = kp;
    pid->ki = ki;
    pid->ka = ka;
    pid->dt = dt;
    pid->out_max = out_max;
    pid->error_sum_max = error_sum_max;

    pid->target = 0.f;
    pid->fdb = 0.f;
    pid->error = 0.f;
    pid->error_sum = 0.f;
    pid->k_out = 0.f;
    pid->i_out = 0.f;
    pid->out = 0.f;
    pid->delta_out = 0.f;
}

float FocAlg_BackCalcPIController_Calc(FocAlg_Pid_s *pid, float target, float fdb) {
    pid->target = target;
    pid->fdb = fdb;
    pid->error = pid->target - pid->fdb;

    pid->error_sum += pid->dt * pid->error;

    pid->k_out = pid->kp * pid->error;
    pid->i_out = pid->ki * pid->error_sum;

    float out_unsat = pid->k_out + pid->i_out;
    float out_sat = FocMath_Clamp(out_unsat, -pid->out_max, pid->out_max);
    pid->delta_out = out_sat - out_unsat;
    pid->out = out_sat;

    // 反计算积分抗饱和
    pid->error_sum = pid->error_sum + pid->ka  * pid->dt * pid->delta_out;

    // 保护防止超出限幅
    pid->error_sum = FocMath_Clamp(pid->error_sum, -pid->error_sum_max, pid->error_sum_max);

    return pid->out;
}

void FocAlg_BackCalaPIController_Clear(FocAlg_Pid_s *pid) {
    if (pid == NULL) return;

    pid->target = 0.f;
    pid->fdb = 0.f;
    pid->error = 0.f;

    pid->error_sum = 0.f;
    pid->k_out = 0.f;
    pid->i_out = 0.f;
    pid->out = 0.f;
    pid->delta_out = 0.f;
}

void FocAlg_TargetRamper_Init(FocAlg_TargetRamper_s* ramper, float slope, float dt, float initial_target) {
    if (ramper == NULL) return;
    ramper->slope = slope;
    ramper->dt = dt;

    ramper->target = initial_target;
    ramper->out = initial_target;
    ramper->last_out = initial_target;
}

float FocAlg_TargetRamper_Calc(FocAlg_TargetRamper_s* ramper, float target) {
    float delta = target-ramper->last_out;

    if (FOCMATH_ABS(delta) < ramper->slope * ramper->dt ) {
        ramper->out = target;
    }else {
        ramper->out += delta / FOCMATH_ABS(delta) * ramper->slope * ramper->dt;
    }

    ramper->last_out = ramper->out;
    return ramper->out;
}

void FocAlg_TargetRamper_Clear(FocAlg_TargetRamper_s* ramper) {
    if (ramper == NULL) return;

    ramper->target = 0.f;
    ramper->out = 0.f;
    ramper->last_out = 0.f;
}

void FocAlg_SinCosPll_Init(FocAlg_SinCosPll_s* pll,
    float kp, float ki, float ka, float ts, float out_max, float error_sum_max) {
    if (pll == NULL) return;
    pll->kp = kp;
    pll->ki = ki;
    pll->ka = ka;
    pll->ts = ts;
    pll->out_max = out_max;
    pll->error_sum_max = error_sum_max;

    pll->error = 0.f;
    pll->error_sum = 0.f;

    pll->theta_e_hat = 0.f;
    pll->omega_e_raw = 0.f;
    pll->omega_e_hat = 0.f;
    pll->last_theta_e_hat = 0.f;
    pll->theta_e_delta = 0.f;
    pll->is_angle_initialized = false;
    FocAlg_MovingAverageFilter_Init(&pll->speed_filter, 20);
}

void FocAlg_SinCosPll_Calc(FocAlg_SinCosPll_s* pll, float sin, float cos) {
    if (pll == NULL) return;

    if (!pll->is_angle_initialized) {
        pll->theta_e_hat = FocMath_FastAtan(sin, cos);
        pll->last_theta_e_hat = pll->theta_e_hat;
        pll->theta_e_delta = 0.f;
        pll->is_angle_initialized = true;
    }

    pll->error = - cos * FocMath_FastSin(pll->theta_e_hat) +  sin * FocMath_FastCos(pll->theta_e_hat);

    pll->error_sum += pll->error * pll->ts;

    float pout = pll->kp * pll->error;
    float iout = pll->error_sum * pll->ki;
    float omega_unsat = pout + iout;
    pll->omega_e_raw = FocMath_Clamp(omega_unsat, -pll->out_max, pll->out_max);
    float delta_out = pll->omega_e_raw - omega_unsat;

    pll->error_sum = pll->error_sum + pll->ka * delta_out * pll->ts;
    pll->error_sum = FocMath_Clamp(pll->error_sum, -pll->error_sum_max, pll->error_sum_max);

    pll->theta_e_hat += pll->omega_e_raw * pll->ts;
    if (pll->theta_e_hat > DOUBLE_PI) pll->theta_e_hat -= DOUBLE_PI;
    if (pll->theta_e_hat < 0) pll->theta_e_hat += DOUBLE_PI;

    pll->theta_e_delta = pll->theta_e_hat - pll->last_theta_e_hat;
    if (pll->theta_e_delta > PI) pll->theta_e_delta -= DOUBLE_PI;
    else if (pll->theta_e_delta < -PI) pll->theta_e_delta += DOUBLE_PI;
    pll->last_theta_e_hat = pll->theta_e_hat;

    pll->omega_e_hat = FocAlg_MovingAverageFilter_Calc(&pll->speed_filter, pll->omega_e_raw);
}

void FocAlg_DirectEEMF_Init(FocAlg_DirectEEMF_s* eemf,
        float rs, float ls, float ts) {
    eemf->rs = rs;
    eemf->ls = ls;
    eemf->ts = ts;

    memset(eemf->u_alphabeta, 0, sizeof(float) * 2);
    memset(eemf->i_alphabeta, 0, sizeof(float) * 2);
    memset(eemf->last_i_alphabeta, 0, sizeof(float) * 2);

    eemf->theta_e_hat = 0.f;
}

float FocAlg_DirectEEMF_Calc(FocAlg_DirectEEMF_s* eemf,
        float* u_alphabeta, float* i_alphabeta) {
    memcpy(eemf->last_i_alphabeta, eemf->i_alphabeta, sizeof(float) * 2);
    memcpy(eemf->i_alphabeta, i_alphabeta, sizeof(float) * 2);
    memcpy(eemf->u_alphabeta, u_alphabeta, sizeof(float) * 2);

    const float Ld_slash_Ts = eemf->ls / eemf->ts;
    float e_alpha = eemf->u_alphabeta[0] - eemf->rs * eemf->i_alphabeta[0] - Ld_slash_Ts * (eemf->i_alphabeta[0] - eemf->last_i_alphabeta[0]);
    float e_beta = eemf->u_alphabeta[1] - eemf->rs * eemf->i_alphabeta[1] - Ld_slash_Ts * (eemf->i_alphabeta[1] - eemf->last_i_alphabeta[1]);

    eemf->theta_e_hat = FocMath_FastAtan(-e_alpha, e_beta);
    return eemf->theta_e_hat;
}

void FocAlg_PllEEMF_Init(FocAlg_PllEEMF_s* eemf,
    float rs, float ls, float ts, float kp, float ki, float ka, float out_max, float i_max, float omega_filter_k) {
    eemf->rs = rs;
    eemf->ls = ls;
    eemf->ts = ts;
    eemf->kp = kp;
    eemf->ki = ki;
    eemf->ka = ka;
    eemf->out_max = out_max;
    eemf->error_sum_max = i_max;
    eemf->omega_filter_k = omega_filter_k;
    FocAlg_MovingAverageFilter_Init(&eemf->speed_filter, 20);

    eemf->error_sum = 0;
    memset(eemf->u_alphabeta, 0, sizeof(float) * 2);
    memset(eemf->i_alphabeta, 0, sizeof(float) * 2);
    memset(eemf->last_i_alphabeta, 0, sizeof(float) * 2);

    eemf->omega_e_raw = 0.f;
    eemf->omega_m_raw = 0.f;
    eemf->last_theta_e_hat = 0.f;
    eemf->p_out = 0.f;
    eemf->i_out = 0.f;
    eemf->theta_e_hat = 0.f;
    eemf->omega_e_hat = 0.f;
}

void FocAlg_PllEEMF_Calc(FocAlg_PllEEMF_s* eemf,
        float* u_alphabeta, float* i_alphabeta) {
    memcpy(eemf->last_i_alphabeta, eemf->i_alphabeta, sizeof(float) * 2);
    memcpy(eemf->i_alphabeta, i_alphabeta, sizeof(float) * 2);
    memcpy(eemf->u_alphabeta, u_alphabeta, sizeof(float) * 2);

    const float Ld_slash_Ts = eemf->ls / eemf->ts;
    float e_alpha = eemf->u_alphabeta[0] - eemf->rs * eemf->i_alphabeta[0] - Ld_slash_Ts * (eemf->i_alphabeta[0] - eemf->last_i_alphabeta[0]);
    float e_beta = eemf->u_alphabeta[1] - eemf->rs * eemf->i_alphabeta[1] - Ld_slash_Ts * (eemf->i_alphabeta[1] - eemf->last_i_alphabeta[1]);

    eemf->theta_e_raw = FocMath_FastAtan(-e_alpha, e_beta);

    // 尝试下来计算atan2得到角度再计算误差的噪声比使用sin计算噪声更小一些，实际原因应该是量纲没有对齐导致的
    // float error = -e_alpha * FocMath_FastCos(eemf->theta_e_hat) - e_beta * FocMath_FastSin(eemf->theta_e_hat);
    float error = eemf->theta_e_raw - eemf->theta_e_hat;
    if (error > PI) error -= DOUBLE_PI;
    else if (error < - PI) error += DOUBLE_PI;

    eemf->error_sum += error * eemf->ts;
    eemf->p_out = error * eemf->kp;
    eemf->i_out = eemf->error_sum * eemf->ki;
    float omega_unsat = error * eemf->kp + eemf->error_sum * eemf->ki;
    eemf->omega_e_raw = FocMath_Clamp(omega_unsat, -eemf->out_max, eemf->out_max);
    eemf->out_delta = eemf->omega_e_raw - omega_unsat;

    // 反计算抗饱和
    eemf->error_sum = eemf->error_sum + eemf->ka * eemf->ts * eemf->out_delta;

    eemf->error_sum = FocMath_Clamp(eemf->error_sum, -eemf->error_sum_max, eemf->error_sum_max);

    // 角度积分
    eemf->theta_e_hat += eemf->omega_e_raw * eemf->ts;
    if (eemf->theta_e_hat < 0) eemf->theta_e_hat += DOUBLE_PI;
    else if (eemf->theta_e_hat > DOUBLE_PI) eemf->theta_e_hat -= DOUBLE_PI;

    // 速度滤波 滑动均值
    eemf->omega_e_hat = FocAlg_MovingAverageFilter_Calc(&eemf->speed_filter, eemf->omega_e_raw);
    eemf->last_theta_e_hat = eemf->theta_e_hat;
}

void FocAlg_SmoEEMF_Init(FocAlg_SmoEEMF_s* eemf,
        float rs, float ls, float ts, float gain_h, float e_max) {
    eemf->rs = rs;
    eemf->ls = ls;
    eemf->ts = ts;
    eemf->gain_h = gain_h;
    eemf->e_max = e_max;

    eemf->inv_ld = 1.f / eemf->ls;
    eemf->r_slash_ls = eemf->rs / eemf->ls;

    memset(eemf->u_alphabeta, 0, sizeof(float) * 2);
    memset(eemf->i_alphabeta, 0, sizeof(float) * 2);
    memset(eemf->i_alphabeta_hat, 0, sizeof(float) * 2);
    memset(eemf->voltage_alphabeta_hat, 0, sizeof(float) * 2);
    FocAlg_FirstOrderLowPassFilter_Init(&eemf->theta_e_filter, 4000.f, ts);

    eemf->theta_e_hat = 0.f;
}

void FocAlg_SmoEEMF_Calc(FocAlg_SmoEEMF_s* eemf,
        float* u_alphabeta, float* i_alphabeta) {
    memcpy(eemf->u_alphabeta, u_alphabeta, sizeof(float) * 2);
    memcpy(eemf->i_alphabeta, i_alphabeta, sizeof(float) * 2);

    eemf->i_alphabeta_hat[0] = eemf->i_alphabeta[0] + eemf->ts *
        (eemf->inv_ld * eemf->u_alphabeta[0] - eemf->r_slash_ls * eemf->i_alphabeta_hat[0] - eemf->inv_ld * eemf->voltage_alphabeta_hat[0]);
    eemf->i_alphabeta_hat[1] = eemf->i_alphabeta[1] + eemf->ts *
        (eemf->inv_ld * eemf->u_alphabeta[1] - eemf->r_slash_ls * eemf->i_alphabeta_hat[1] - eemf->inv_ld * eemf->voltage_alphabeta_hat[1]);

    float err[2] = {};
    err[0] = eemf->i_alphabeta_hat[0] - eemf->i_alphabeta[0];
    err[1] = eemf->i_alphabeta_hat[1] - eemf->i_alphabeta[1];
    float sat_err[2] = {};
    if (FOCMATH_ABS(err[0]) < eemf->e_max) sat_err[0] = err[0];
    else if (err[0] < 0) sat_err[0] = -eemf->e_max;
    else sat_err[0] = eemf->e_max;
    if (FOCMATH_ABS(err[1]) < eemf->e_max) sat_err[1] = err[1];
    else if (err[1] < 0) sat_err[1] = -eemf->e_max;
    else sat_err[1] = eemf->e_max;

    eemf->voltage_alphabeta_hat[0] = eemf->gain_h * sat_err[0];
    eemf->voltage_alphabeta_hat[1] = eemf->gain_h * sat_err[1];

    eemf->theta_e_hat_raw = FocMath_FastAtan(-eemf->voltage_alphabeta_hat[0], eemf->voltage_alphabeta_hat[1]);

    eemf->theta_e_hat = FocAlg_FirstOrderLowPassFilter_AngleCalc(&eemf->theta_e_filter, eemf->theta_e_hat_raw);
}

void FocAlg_PllSmoEEMF_Init(FocAlg_PllSmoEEMF_s* eemf,
    float rs, float ls, float ts, float gain_h, float e_max, float kp, float ki, float ka, float out_max, float i_max) {
    eemf->rs = rs;
    eemf->ls = ls;
    eemf->ts = ts;
    eemf->gain_h = gain_h;
    eemf->e_max = e_max;
    eemf->kp = kp;
    eemf->ki = ki;
    eemf->ka = ka;
    eemf->out_max = out_max;
    eemf->error_sum_max = i_max;
    FocAlg_MovingAverageFilter_Init(&eemf->speed_filter, 20);

    eemf->inv_ld = 1.f / eemf->ls;
    eemf->r_slash_ls = eemf->rs / eemf->ls;

    eemf->error_sum = 0.f;
    eemf->p_out = 0.f;
    eemf->i_out = 0.f;
    eemf->out_delta = 0.f;
    memset(eemf->u_alphabeta, 0, sizeof(float) * 2);
    memset(eemf->i_alphabeta, 0, sizeof(float) * 2);
    memset(eemf->i_alphabeta_hat, 0, sizeof(float) * 2);
    memset(eemf->voltage_alphabeta_hat, 0, sizeof(float) * 2);

    eemf->theta_e_hat_raw = 0.f;
    eemf->theta_e_hat = 0.f;
    eemf->last_theta_e_hat = 0.f;
    eemf->omega_e_hat = 0.f;
    eemf->omega_e_hat_raw = 0.f;
}

void FocAlg_PllSmoEEMF_Calc(FocAlg_PllSmoEEMF_s* eemf,
        float* u_alphabeta, float* i_alphabeta) {

    memcpy(eemf->u_alphabeta, u_alphabeta, sizeof(float) * 2);
    memcpy(eemf->i_alphabeta, i_alphabeta, sizeof(float) * 2);

    eemf->i_alphabeta_hat[0] = eemf->i_alphabeta[0] + eemf->ts *
        (eemf->inv_ld * eemf->u_alphabeta[0] - eemf->r_slash_ls * eemf->i_alphabeta_hat[0] - eemf->inv_ld * eemf->voltage_alphabeta_hat[0]);
    eemf->i_alphabeta_hat[1] = eemf->i_alphabeta[1] + eemf->ts *
        (eemf->inv_ld * eemf->u_alphabeta[1] - eemf->r_slash_ls * eemf->i_alphabeta_hat[1] - eemf->inv_ld * eemf->voltage_alphabeta_hat[1]);

    float err[2] = {};
    err[0] = eemf->i_alphabeta_hat[0] - eemf->i_alphabeta[0];
    err[1] = eemf->i_alphabeta_hat[1] - eemf->i_alphabeta[1];
    float sat_err[2] = {};
    if (FOCMATH_ABS(err[0]) < eemf->e_max) sat_err[0] = err[0];
    else if (err[0] < 0) sat_err[0] = -eemf->e_max;
    else sat_err[0] = eemf->e_max;
    if (FOCMATH_ABS(err[1]) < eemf->e_max) sat_err[1] = err[1];
    else if (err[1] < 0) sat_err[1] = -eemf->e_max;
    else sat_err[1] = eemf->e_max;

    eemf->voltage_alphabeta_hat[0] = eemf->gain_h * sat_err[0];
    eemf->voltage_alphabeta_hat[1] = eemf->gain_h * sat_err[1];

    eemf->theta_e_hat_raw = FocMath_FastAtan(-eemf->voltage_alphabeta_hat[0], eemf->voltage_alphabeta_hat[1]);

    // 归一化后 error = sin(theta_raw - theta_hat)，量纲为 1，而不是V
    float emf_amp = FocMath_FastSqrt(
        eemf->voltage_alphabeta_hat[0] * eemf->voltage_alphabeta_hat[0]
        + eemf->voltage_alphabeta_hat[1] * eemf->voltage_alphabeta_hat[1]);
    float error = 0.f;
    if (emf_amp > 0.01f) {
        error = (-eemf->voltage_alphabeta_hat[0] * FocMath_FastCos(eemf->theta_e_hat)
            - eemf->voltage_alphabeta_hat[1] * FocMath_FastSin(eemf->theta_e_hat)) / emf_amp;
    }

    eemf->error_sum += error * eemf->ts;
    eemf->p_out = error * eemf->kp;
    eemf->i_out = eemf->error_sum * eemf->ki;
    float omega_unsat = error * eemf->kp + eemf->error_sum * eemf->ki;
    eemf->omega_e_hat_raw = FocMath_Clamp(omega_unsat, -eemf->out_max, eemf->out_max);
    eemf->out_delta = eemf->omega_e_hat_raw - omega_unsat;

    // 反计算抗饱和
    eemf->error_sum = eemf->error_sum + eemf->ka * eemf->ts * eemf->out_delta;

    eemf->error_sum = FocMath_Clamp(eemf->error_sum, -eemf->error_sum_max, eemf->error_sum_max);

    // 角度积分
    eemf->theta_e_hat += eemf->omega_e_hat_raw * eemf->ts;
    if (eemf->theta_e_hat < 0) eemf->theta_e_hat += DOUBLE_PI;
    else if (eemf->theta_e_hat > DOUBLE_PI) eemf->theta_e_hat -= DOUBLE_PI;

    // 速度滤波 滑动均值
    eemf->omega_e_hat = FocAlg_MovingAverageFilter_Calc(&eemf->speed_filter, eemf->omega_e_hat_raw);
    eemf->last_theta_e_hat = eemf->theta_e_hat;
}

void FocAlg_MovingAverageFilter_Init(FocAlg_MovingAverageFilter_s* filter, uint8_t length) {
    filter->length = length;
    if (length == 0) filter->length = 1;
    if (length > 30) filter->length = 30;

    filter->is_full = false;
    memset(filter->buffer, 0, sizeof(filter->buffer));
    filter->cnt = 0;
    filter->average = 0.f;
}

float FocAlg_MovingAverageFilter_Calc(FocAlg_MovingAverageFilter_s* filter, float input) {
    float old_sample = filter->buffer[filter->cnt % filter->length];
    filter->buffer[filter->cnt % filter->length] = input;
    if (filter->cnt >= filter->length && filter->is_full == false) filter->is_full = true;
    filter->cnt = (filter->cnt >= filter->length) ? 0 : filter->cnt + 1;

    if (filter->is_full) {
        filter->average += (input - old_sample) / (float)filter->length;
        return filter->average;
    }else {
        filter->average = (filter->average * ((float)filter->cnt - 1) + input) / (float)filter->cnt;
        return filter->average;
    }
}

void FocAlg_FirstOrderLowPassFilter_Init(FocAlg_FirstOrderLowPassFilter_s* filter, float cut_off_freq, float ts) {
    if (filter == NULL) return;
    filter->alpha = 1.f - expf(-DOUBLE_PI * cut_off_freq * ts);
    filter->is_init = false;

    filter->out = 0.f;
    filter->last_out = 0.f;
}

float FocAlg_FirstOrderLowPassFilter_Calc(FocAlg_FirstOrderLowPassFilter_s* filter, float input) {
    if (!filter->is_init) {
        filter->out = input;
        filter->last_out = input;
        filter->is_init = true;

        return filter->out;
    }else {
        filter->out = (1 - filter->alpha) * filter->last_out + filter->alpha * input;
        filter->last_out = filter->out;
        return filter->out;
    }
}

static float FocAlg_WrapPi(float angle) {
    if (angle > PI) {
        angle -= DOUBLE_PI;
    } else if (angle < -PI) {
        angle += DOUBLE_PI;
    }
    return angle;
}

static float FocAlg_Wrap2Pi(float angle) {
    if (angle >= DOUBLE_PI) {
        angle -= DOUBLE_PI;
    } else if (angle < 0.0f) {
        angle += DOUBLE_PI;
    }
    return angle;
}

float FocAlg_FirstOrderLowPassFilter_AngleCalc(FocAlg_FirstOrderLowPassFilter_s* filter, float input) {
    input = FocAlg_Wrap2Pi(input);

    if (!filter->is_init) {
        filter->out = input;
        filter->last_out = input;
        filter->is_init = true;

        return filter->out;
    }

    float error = FocAlg_WrapPi(input - filter->last_out);
    float output = filter->last_out + filter->alpha * error;

    output = FocAlg_Wrap2Pi(output);

    filter->out = output;
    filter->last_out = output;
    return output;
}

