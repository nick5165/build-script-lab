#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <iomanip>
#include <cstdint>

using namespace std;

class SHA1 {
public:
    SHA1() { reset(); }
    
    void update(istream &is) {
        char sbuf[2048];
        while (is.read(sbuf, sizeof(sbuf)) || is.gcount()) {
            size_t count = is.gcount();
            for (size_t i = 0; i < count; ++i) add_byte(sbuf[i]);
        }
    }
    
    string final() {
        process_buffer_padding();
        stringstream ss;
        for (int i = 0; i < 5; ++i) {
            ss << hex << setw(8) << setfill('0') << h[i];
        }
        return ss.str();
    }

private:
    uint32_t h[5];
    unsigned char buffer[64];
    uint64_t size;
    uint32_t buffer_idx;

    void reset() {
        h[0] = 0x67452301; h[1] = 0xEFCDAB89; h[2] = 0x98BADCFE; h[3] = 0x10325476; h[4] = 0xC3D2E1F0;
        size = 0; buffer_idx = 0;
    }
    
    void add_byte(unsigned char byte) {
        buffer[buffer_idx++] = byte;
        size++;
        if (buffer_idx == 64) process_block(buffer);
    }
    
    void process_block(const unsigned char* block) {
        uint32_t w[80];
        for (int i = 0; i < 16; i++)
            w[i] = (block[i*4] << 24) | (block[i*4+1] << 16) | (block[i*4+2] << 8) | (block[i*4+3]);
        for (int i = 16; i < 80; i++)
            w[i] = left_rotate(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
        
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
        for (int i = 0; i < 80; i++) {
            uint32_t f, k;
            if (i < 20) { f = (b & c) | ((~b) & d); k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6; }
            uint32_t temp = left_rotate(a, 5) + f + e + k + w[i];
            e = d; d = c; c = left_rotate(b, 30); b = a; a = temp;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
        buffer_idx = 0;
    }
    
    void process_buffer_padding() {
        unsigned char temp_buffer[64];
        copy(buffer, buffer + buffer_idx, temp_buffer);
        size_t temp_idx = buffer_idx;
        temp_buffer[temp_idx++] = 0x80;
        if (temp_idx > 56) {
            fill(temp_buffer + temp_idx, temp_buffer + 64, 0);
            process_block(temp_buffer);
            temp_idx = 0;
        }
        fill(temp_buffer + temp_idx, temp_buffer + 56, 0);
        uint64_t bit_size = size * 8;
        for (int i = 0; i < 8; i++) temp_buffer[63 - i] = (bit_size >> (i * 8)) & 0xFF;
        process_block(temp_buffer);
    }
    
    uint32_t left_rotate(uint32_t n, unsigned int d) { return (n << d) | (n >> (32 - d)); }
};

string getFileHash(const filesystem::path& path) {
    ifstream file(path, ios::binary);
    if (!file.is_open()) return "";
    
    SHA1 checksum;
    checksum.update(file);
    return checksum.final();
}

int main() {
    setlocale(LC_ALL, "");

    string inputPath;
    cout << "Введите путь к каталогу: ";
    getline(cin, inputPath);

    filesystem::path rootPath(inputPath);

    if (!filesystem::exists(rootPath) || !filesystem::is_directory(rootPath)) {
        cerr << "Ошибка: Путь не существует или это не папка." << endl;
        return 1;
    }

    map<string, filesystem::path> database;
    
    cout << "Сканирование..." << endl;

    for (const auto& entry : filesystem::recursive_directory_iterator(rootPath)) {
        
        if (entry.is_regular_file()) {
            filesystem::path currentPath = entry.path();
            
            string hash = getFileHash(currentPath);
            if (hash.empty()) continue;

            if (database.find(hash) == database.end()) {
                database[hash] = currentPath;
            } else {
                filesystem::path originalPath = database[hash];

                if (filesystem::equivalent(currentPath, originalPath)) {
                    continue;
                }

                cout << "[Дубликат] " << currentPath.filename() 
                     << " -> замена на ссылку к " << originalPath.filename() << endl;

                try {
                    filesystem::remove(currentPath);
                    filesystem::create_hard_link(originalPath, currentPath);
                } catch (const filesystem::filesystem_error& e) {
                    cerr << "Ошибка файловой системы: " << e.what() << endl;
                }
            }
        }
    }

    cout << "Готово!" << endl;
    return 0;
}