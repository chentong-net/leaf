//
// Created by Chen Tong on 2026/2/5.
//

#ifndef PROFILESECURITY_H
#define PROFILESECURITY_H

#include <cstdint>
#include <vector>

namespace ProfileSecurity {

    inline void xtea_decrypt(uint32_t v[2], const uint32_t k[4]) {
        uint32_t v0 = v[0], v1 = v[1];
        uint32_t sum = 0xC6EF3720; // delta * 32
        uint32_t delta = 0x9E3779B9;

        for (int i = 0; i < 32; i++) {
            v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
            sum -= delta;
            v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
        }
        v[0] = v0; v[1] = v1;
    }

    inline std::string decrypt_profile_data(const std::vector<unsigned char>& encryptedData) {
        LF_LOGI("Decrypting size: %zu", encryptedData.size());
        if (encryptedData.size() >= 8) {
            // 打印前8个字节的十六进制
            LF_LOGI("Header Hex: %02X %02X %02X %02X | %02X %02X %02X %02X",
                encryptedData[0], encryptedData[1], encryptedData[2], encryptedData[3],
                encryptedData[4], encryptedData[5], encryptedData[6], encryptedData[7]);
        }
        if (encryptedData.size() < 4) return "";
        uint32_t originalLen = 0;
        originalLen |= ((uint32_t)(uint8_t)encryptedData[0]);
        originalLen |= ((uint32_t)(uint8_t)encryptedData[1] << 8);
        originalLen |= ((uint32_t)(uint8_t)encryptedData[2] << 16);
        originalLen |= ((uint32_t)(uint8_t)encryptedData[3] << 24);
        uint32_t key[4];
        key[0] = 0x00000000;
        key[1] = 0x00000000;
        key[2] = 0x00000000;
        key[3] = 0x00000000;
        const uint8_t* src = encryptedData.data() + 4;
        size_t srcLen = encryptedData.size() - 4;
        std::vector<uint8_t> buffer(srcLen);
        for (size_t i = 0; i < srcLen; i += 8) {
            uint32_t block[2];
            block[0] = src[i] | (src[i+1] << 8) | (src[i+2] << 16) | (src[i+3] << 24);
            block[1] = src[i+4] | (src[i+5] << 8) | (src[i+6] << 16) | (src[i+7] << 24);
            xtea_decrypt(block, key);
            uint32_t d0 = block[0];
            uint32_t d1 = block[1];
            buffer[i]   = d0 & 0xFF; buffer[i+1] = (d0>>8) & 0xFF; buffer[i+2] = (d0>>16) & 0xFF; buffer[i+3] = (d0>>24) & 0xFF;
            buffer[i+4] = d1 & 0xFF; buffer[i+5] = (d1>>8) & 0xFF; buffer[i+6] = (d1>>16) & 0xFF; buffer[i+7] = (d1>>24) & 0xFF;
        }
        if (originalLen > buffer.size()) return "";
        return std::string(buffer.begin(), buffer.begin() + originalLen);
    }
}

#endif // PROFILESECURITY_H
