#include "pch.h"
#include "lol_before.h"

#include <nlohmann/json.hpp>
void tset01() {

    //int danshuangRanked = 2;
    //int danshuangScore = 3;
    //int linghuoRanked = 11;
    //int linghuoScore = 5;

    //// ���� JSON
    //nlohmann::json j;
    //j["danshuang"]["ranked"] = danshuangRanked;
    //j["danshuang"]["score"] = danshuangScore;
    //j["linghuo"]["ranked"] = linghuoRanked;
    //j["linghuo"]["score"] = linghuoScore;

    //// תΪ�ַ���
    //std::string jsonStr = j.dump(4); // ����4��Ư�����

    //std::cout << jsonStr << std::endl;
}

extern "C" __declspec(dllexport) void test00(int a, int b) {
    Game_Before gb;
    gb.before_main("");
}

