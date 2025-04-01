#include "dxgi.h"
#include "helpers.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iostream>
#include <stdio.h>
#include <string>
#include <tchar.h>
#include <time.h>
#include <windows.h>

#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include <Strsafe.h>

using namespace std;

extern "C" {
    bool result_flg = false;
    wchar_t dllDirectory[MAX_PATH] = { 0 };

    // DLLのロード時に呼ばれる関数
    BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
    {
        if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        {
            if (GetModuleFileName(hModule, dllDirectory, MAX_PATH) > 0)
            {
                PathRemoveFileSpec(dllDirectory); // ディレクトリ部分のみを残す
            }
        }
        return TRUE;
    }

    // UTF-8 → Shift-JIS に戻す
    std::string UTF8ToShiftJIS(const std::string& utf8Str) {
        int wLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
        if (wLen == 0) return "";

        std::wstring wstr(wLen, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wstr[0], wLen);

        int sjisLen = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
        if (sjisLen == 0) return "";

        std::string sjisStr(sjisLen, 0);
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &sjisStr[0], sjisLen, NULL, NULL);

        return sjisStr;
    }

    // Shift-JIS → UTF-8 に変換
    std::string ShiftJISToUTF8(const std::string& sjisStr) {
        int wLen = MultiByteToWideChar(CP_ACP, 0, sjisStr.c_str(), -1, NULL, 0);
        if (wLen == 0) return "";

        std::wstring wstr(wLen, 0);
        MultiByteToWideChar(CP_ACP, 0, sjisStr.c_str(), -1, &wstr[0], wLen);

        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
        if (utf8Len == 0) return "";

        std::string utf8Str(utf8Len, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8Str[0], utf8Len, NULL, NULL);

        return utf8Str;
    }

    double truncateToTwoDecimalPlaces(double value) {
        return std::floor(value * 100) / 100;  // 小数第3位以下を切り捨て
    }

    void __declspec (dllexport) OnFrame(IDXGISwapChain* swapChain)
    {
        if (result_flg == false) 
        {
            // リザルト(クリアもしくはゲームオーバー)画面になった
            int result = READ_MEMORY(0x1412EF4C0, uint32_t);
            int gameover = READ_MEMORY(0x14CC08E8C, uint32_t);
            int gameover2 = READ_MEMORY(0x14CC08E98, uint32_t);
            if (result >= 1 || gameover == 1 || gameover2 == 1)
            {
                // PV鑑賞ではない
                if (READ_MEMORY(0x14CC6E41C, uint32_t) == 0 && READ_MEMORY(0x14CC6E42B, uint32_t) == 0)
                {
                    // 通常モード（完奏モードではない）
                    if (READ_MEMORY(0x1416E2BA8, uint32_t) == 0)
                    {
                        double tasseiritu = READ_MEMORY(0x1412EF634, float);
                        tasseiritu = trunc(tasseiritu * 100);
                        tasseiritu = tasseiritu / 100;

                        long score = READ_MEMORY(0x1412EF568, int_fast32_t);
                        int diff = READ_MEMORY(0x1416E2B90, uint32_t);
                        int exExt = READ_MEMORY(0x1416E2B94, uint32_t);
                        float star = (float)(READ_MEMORY(0x1416E2BFC, uint32_t)) / 2;
                        int life = READ_MEMORY(0x1412EF564, uint32_t);

                        // 曲名(表示)
                        //char* song_name = READ_MEMORY(0x14CC0B5F8, char*);
                        //std::string sjis_song_name = UTF8ToShiftJIS(song_name);     // 海外言語だとクラッシュする
                        //cout << "[DLL1 enomoto] 楽曲名 : " << sjis_song_name << endl;

                        char* song_name = READ_MEMORY(0x14CC0B5F8, char*);

                        wstring fileName = L"/PlayRecord.txt";
                        wstring filePath = dllDirectory + fileName;

                        ofstream outputfile(filePath, ios::app);    // ios::appで追記
                        if (outputfile.is_open()) {
                            try
                            {
                                char nowTime[26];
                                time_t datetime = time(NULL);
                                struct tm local;
                                localtime_s(&local, &datetime);
                                ctime_s(nowTime, 26, &datetime);
                                string now = nowTime;
                                now.pop_back();
                                outputfile << now << "\t" << song_name << "\t" << diff << "\t" << exExt << "\t" << star << "\t" << tasseiritu << "\t" << score << "\t" << result << "\n";
                                outputfile.close();

                            }
                            catch (const std::ios_base::failure& e) {
                                throw std::runtime_error("[DLL1 enomoto] File reading error: " + std::string(e.what()));
                            }
                        }
                        else
                        {
                            cout << "[DLL1 enomoto] Failed to record play history. Could not open the file.";
                        }

                        result_flg = true;
                    }
                }
            }
        }
        else
        {
            // リザルト画面が終わった
            if (READ_MEMORY(0x1412EE3C0, uint32_t) == 0 && READ_MEMORY(0x14CC08E8C, uint32_t) == 0)
            {
                result_flg = false;
            }
        }
        
    }
}