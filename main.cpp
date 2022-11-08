#include <iostream>
#include <bitset>
#include <fstream>

#define byte unsigned char
#define ROUND_COUNT 31
using namespace std;

class Present {

public:
    bitset<80> key;
    static byte S(byte x) {
        byte array[16]{0xC, 0x5, 0x6, 0xb, 0x9, 0x0, 0xa, 0xd, 0x3,
                0xe, 0xf, 0x8, 0x4, 0x7, 0x1, 0x2};
        return array[int(x)];
    }
    static byte antiS(byte x) {
        byte array[16] {0x5, 0xe, 0xf, 0x8, 0xc, 0x1, 0x2, 0xd, 0xb, 0x4,
                        0x6, 0x3, 0x0, 0x7, 0x9, 0xa
        };
        return array[int(x)];
    }
    static byte P(byte x) {
        return (x * 16 + x / 4) % 64;
    }
    static byte antiP(byte x) {
        byte array[64];
        for (int i = 0; i < 64; ++i) {
            array[(i * 16 + i / 4) % 64] = i;
        }
        return array[x];
    }
    static void sLayer(bitset<64> &bits) {
        for (int i = 0; i < 16; ++i) {
            bitset<4> b;
            for (int j = 0; j < 4; ++j) {
                b[j] = bits[i * 4 + j];
            }
            byte temp = (byte) b.to_ulong();
            temp = S(temp);
            bitset<4> b2(temp);
            for (int j = 0; j < 4; ++j) {
                bits[i * 4 + j] = b2[j];
            }
        }
    }
    static void antiSLayer(bitset<64> &bits) {
        for (int i = 0; i < 16; ++i) {
            bitset<4> b;
            for (int j = 0; j < 4; ++j) {
                b[j] = bits[i * 4 + j];
            }
            byte temp = (byte) b.to_ulong();
            temp = antiS(temp);
            bitset<4> b2(temp);
            for (int j = 0; j < 4; ++j) {
                bits[i * 4 + j] = b2[j];
            }
        }
    }
    std::bitset<8 * 8> pLayer(bitset<64> bits) {
        bitset<64> res(0);
        for (int i = 0; i < 64; ++i) {
            res[P(i)] = bits[i];
        }
        return res;
    }
    std::bitset<8 * 8> antiPLayer(bitset<64> bits) {
        bitset<64> res(0);
        for (int i = 0; i < 64; ++i) {
            res[antiP(i)] = bits[i];
        }
        return res;
    }

    void keySchedule(uint8_t round) {
        // first step
        // k79..k0 -> k18k17..k20k19
        bitset<80> left = key << (80 - 18);
        key >>= 18;
        key |= left;
        // second step
        bitset<32> temp(0);
        for (int i = 0; i < 4; ++i) {
            temp[i] = key[76 + i];
        }
        byte t = (byte) temp.to_ulong();
        bitset<8> temp2(S(t));
        for (int i = 0; i < 4; ++i) {
            key[76 + i] = temp2[i];
        }
        // third step
        bitset<8> temp3(0);
        for (int i = 0; i < 5; ++i) {
            temp3[i] = key[15 + i];
        }
        byte q = (byte)temp3.to_ulong();
        q ^= round;
        bitset<8> temp4(q);
        for (int i = 0; i < 5; ++i) {
            key[15 + i] = temp4[i];
        }
    }
    void reverseKeySchedule(uint8_t round) {
        bitset<8> temp3(0);
        for (int i = 0; i < 5; ++i) {
            temp3[i] = key[15 + i];
        }
        byte q = (byte)temp3.to_ulong();
        q ^= round;
        bitset<8> temp4(q);
        for (int i = 0; i < 5; ++i) {
            key[15 + i] = temp4[i];
        }
        // second step
        bitset<32> temp(0);
        for (int i = 0; i < 4; ++i) {
            temp[i] = key[76 + i];
        }
        byte t = (byte) temp.to_ulong();
        bitset<8> temp2(antiS(t));
        for (int i = 0; i < 4; ++i) {
            key[76 + i] = temp2[i];
        }
        // first step
        // k79..k0 -> k18k17..k20k19
        bitset<80> left = key >> (80 - 18);
        key <<= 18;
        key |= left;
    }

    template<size_t numBytes>
    std::bitset<numBytes * 8> bytesToBitset(uint8_t *data)
    {
        std::bitset<numBytes * 8> b;

        for(int i = 0; i < numBytes; ++i)
        {
            uint8_t cur = data[i];
            int offset = i * 8;

            for(int bit = 0; bit < 8; ++bit)
            {
                b[offset] = cur & 1;
                ++offset;   // Move to next bit in b
                cur >>= 1;  // Move to next bit in array
            }
        }

        return b;
    }

    Present(const bitset<80> &key) : key(key) {}

    void encode(string input_file, string output_file) {
        ifstream fin;
        fin.open(input_file, ios::binary);
        ofstream fout;
        fout.open(output_file, ios::binary);

        byte block[8];
        int bytes = 7;
        while (!fin.eof()) {
            int i;
            for (i = 0; i < 8; ++i) {
                if (fin.eof()) {
                    bytes = i - 1;
                    if (bytes == -1) {
                        bytes = 7;
                    }
                    break;
                }
                fin.read(reinterpret_cast<char *>(&(block[i])), 1);
            }
            for (; i < 8; ++i) {
                block[i] = 0;
            }
            //xor
            auto b = this->bytesToBitset<8>(block);

            for (int j = 0; j < ROUND_COUNT; ++j) {
                for (int k = 0; k < 64; ++k) {
                    b[k] = b[k] ^ key[k];
                }
                sLayer(b);
                b = pLayer(b);
                keySchedule(j);
            }
            uint64_t o = b.to_ullong();
            fout.write(reinterpret_cast<const char*>(&o), sizeof o);
        }
        uint64_t leastBytes = bytes;
        fout.write(reinterpret_cast<const char *>(&leastBytes), sizeof leastBytes);
        fin.close();
        fout.close();
    }
    void decode(string input_file, string output_file) {
        ifstream fin;
        fin.open(input_file, ios::binary);
        ofstream fout;
        fout.open(output_file, ios::binary);
        fin.seekg(0, fin.end);
        long long end = fin.tellg();
        fin.close();
        fin.open(input_file, ios::binary);
        long long cur_pos = 0;
        uint64_t block;

        int round = 0;
        while (!fin.eof()) {
            fin.read((char*)&block, sizeof(block));
            cur_pos += sizeof(block);
            bitset<64> bits(block);
            if(cur_pos + sizeof(block) == end) {
                uint64_t block2;
                fin.read((char*)&block2, sizeof(block2));
                for (int i = 0; i < ROUND_COUNT; ++i) {
                    keySchedule(i);
                }
                for (int k = 0; k < ROUND_COUNT; ++k) {
                    bits = antiPLayer(bits);
                    antiSLayer(bits);
                    reverseKeySchedule(ROUND_COUNT - k - 1);
                    for (int j = 0; j < 64; ++j) {
                        bits[j] = bits[j] ^ key[j];
                    }
                }
                for (int i = 0; i < ROUND_COUNT; ++i) {
                    keySchedule(i);
                }
                uint64_t o = bits.to_ullong();
                char *output = reinterpret_cast<char*>(&o);
                for (int i = 0; i < block2; ++i) {
                    fout.write(reinterpret_cast<const char *>(&output[i]), 1);
                }
                break;
            }
            for (int i = 0; i < ROUND_COUNT; ++i) {
                keySchedule(i);
            }
            for (int k = 0; k < ROUND_COUNT; ++k) {
                bits = antiPLayer(bits);
                antiSLayer(bits);
                reverseKeySchedule(ROUND_COUNT - k - 1);
                for (int j = 0; j < 64; ++j) {
                    bits[j] = bits[j] ^ key[j];
                }
            }
            for (int i = 0; i < ROUND_COUNT; ++i) {
                keySchedule(i);
            }
            uint64_t o = bits.to_ullong();
            char *output = reinterpret_cast<char*>(&o);
            fout.write(output, sizeof o);
        }
        fin.close();
        fout.close();
    }

    void setKey(const bitset<80> &key) {
        Present::key = key;
    }

    uint64_t hash(string textFile) {
        ifstream fin;
        fin.open(textFile, ios::binary);
        uint64_t res = 0;
        byte block[8];
        int round = 0;
        while (!fin.eof()) {
            int i;
            for (i = 0; i < 8; ++i) {
                if (fin.eof()) {
                    break;
                }
                fin.read(reinterpret_cast<char *>(&(block[i])), 1);
            }
            for (; i < 8; ++i) {
                block[i] = 0;
            }
            //xor
            auto key = this->bytesToBitset<8>(block);
            bitset<64> b(res);
            for (int j = 0; j < ROUND_COUNT; ++j) {
                for (int j = 0; j < 64; ++j) {
                    b[j] = b[j] ^ key[j];
                }
                sLayer(b);
                b = pLayer(b);
                keySchedule(round++);
            }

            uint64_t o = b.to_ullong();
            res ^= o;
        }
        fin.close();

        return res;
    }
};

int main() {
    bitset<80> key;
    for (int i = 0; i < 80; ++i) {
        key[i] = rand() % 2;
    }
    Present p(key);

    p.encode("test.txt", "encode.txt");
    p.setKey(key);

    p.decode("encode.txt", "decode.txt");
    p.setKey(key);
    cout << p.hash("test.txt");
    return 0;
}
