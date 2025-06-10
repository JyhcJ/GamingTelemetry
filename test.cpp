#include "pch.h"
#include "lol_before.h"

#include <nlohmann/json.hpp>
void tset01() {

    //int danshuangRanked = 2;
    //int danshuangScore = 3;
    //int linghuoRanked = 11;
    //int linghuoScore = 5;

    //// 构造 JSON
    //nlohmann::json j;
    //j["danshuang"]["ranked"] = danshuangRanked;
    //j["danshuang"]["score"] = danshuangScore;
    //j["linghuo"]["ranked"] = linghuoRanked;
    //j["linghuo"]["score"] = linghuoScore;

    //// 转为字符串
    //std::string jsonStr = j.dump(4); // 缩进4，漂亮输出

    //std::cout << jsonStr << std::endl;
}

extern "C" __declspec(dllexport) void test00(int a, int b) {
    Game_Before gb;
    gb.before_main("");
}

