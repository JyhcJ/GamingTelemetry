#include "pch.h"
//#include <windows.h>
//#include <iostream>
//#include <string>
//#include <filesystem>
//#include <cstdlib>
//#include <vector>
//#include "py.h"
//
//namespace fs = std::filesystem;
//
//// 解压嵌入式 Python
//bool DeployPython(const std::wstring& zipPath, const std::wstring& targetDir) {
//    // 确保目标目录存在
//    fs::create_directories(targetDir);
//
//    // 使用系统自带的 tar 解压（Win10+ 支持）
//    std::wstring cmd = L"tar -xf \"" + zipPath + L"\" -C \"" + targetDir + L"\"";
//    DWORD ret = ::_wsystem(cmd.c_str());
//    return ret == 0;
//}
//
//// 添加 Python 到环境变量
//void AddPythonToPath(const std::wstring& pythonDir) {
//    // 获取当前环境变量
//    std::wstring path;
//    wchar_t* pathEnv;
//    _wdupenv_s(&pathEnv, nullptr, L"PATH");
//    if (pathEnv) {
//        path = pathEnv;
//        free(pathEnv);
//    }
//
//    // 避免重复添加
//    if (path.find(pythonDir) == std::wstring::npos) {
//        path = pythonDir + L";" + path;
//        _wputenv_s(L"PATH", path.c_str());
//    }
//}
//
//// 验证 Python 是否可用
//bool TestPython() {
//    // 执行 python --version
//    FILE* pipe = _wpopen(L"python --version", L"r");
//    if (!pipe) return false;
//
//    char buffer[128];
//    bool found = false;
//    while (fgets(buffer, sizeof(buffer), pipe)) {
//        if (strstr(buffer, "Python 3")) {
//            found = true;
//            break;
//        }
//    }
//    _pclose(pipe);
//    return found;
//}
//
//// 主函数
//int pymain() {
//    // 1. 配置路径（假设嵌入式 Python ZIP 和程序在同一目录）
//    const std::wstring pythonZip = L"python-3.10.10-embed-amd64.zip";
//    const std::wstring tempDir = L".\\python_temp";
//
//    // 2. 部署 Python
//    std::wcout << L"正在解压 Python..." << std::endl;
//    if (!DeployPython(pythonZip, tempDir)) {
//        std::wcerr << L"错误：解压 Python 失败" << std::endl;
//        return 1;
//    }
//
//    // 3. 设置环境变量
//    AddPythonToPath(fs::absolute(tempDir).wstring());
//    std::wcout << L"已添加临时环境变量" << std::endl;
//
//    // 4. 验证 Python
//    std::wcout << L"验证 Python 安装..." << std::endl;
//    if (TestPython()) {
//        std::wcout << L"成功：Python 已可用" << std::endl;
//
//        // 示例：运行 Python 脚本
//        system("python -c \"print('Hello from Python!')\"");
//    }
//    else {
//        std::wcerr << L"错误：Python 验证失败" << std::endl;
//        return 2;
//    }
//
//    // 5. 可选清理
//    // fs::remove_all(tempDir);
//
//    return 0;
//}