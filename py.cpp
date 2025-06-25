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
//// ��ѹǶ��ʽ Python
//bool DeployPython(const std::wstring& zipPath, const std::wstring& targetDir) {
//    // ȷ��Ŀ��Ŀ¼����
//    fs::create_directories(targetDir);
//
//    // ʹ��ϵͳ�Դ��� tar ��ѹ��Win10+ ֧�֣�
//    std::wstring cmd = L"tar -xf \"" + zipPath + L"\" -C \"" + targetDir + L"\"";
//    DWORD ret = ::_wsystem(cmd.c_str());
//    return ret == 0;
//}
//
//// ��� Python ����������
//void AddPythonToPath(const std::wstring& pythonDir) {
//    // ��ȡ��ǰ��������
//    std::wstring path;
//    wchar_t* pathEnv;
//    _wdupenv_s(&pathEnv, nullptr, L"PATH");
//    if (pathEnv) {
//        path = pathEnv;
//        free(pathEnv);
//    }
//
//    // �����ظ����
//    if (path.find(pythonDir) == std::wstring::npos) {
//        path = pythonDir + L";" + path;
//        _wputenv_s(L"PATH", path.c_str());
//    }
//}
//
//// ��֤ Python �Ƿ����
//bool TestPython() {
//    // ִ�� python --version
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
//// ������
//int pymain() {
//    // 1. ����·��������Ƕ��ʽ Python ZIP �ͳ�����ͬһĿ¼��
//    const std::wstring pythonZip = L"python-3.10.10-embed-amd64.zip";
//    const std::wstring tempDir = L".\\python_temp";
//
//    // 2. ���� Python
//    std::wcout << L"���ڽ�ѹ Python..." << std::endl;
//    if (!DeployPython(pythonZip, tempDir)) {
//        std::wcerr << L"���󣺽�ѹ Python ʧ��" << std::endl;
//        return 1;
//    }
//
//    // 3. ���û�������
//    AddPythonToPath(fs::absolute(tempDir).wstring());
//    std::wcout << L"�������ʱ��������" << std::endl;
//
//    // 4. ��֤ Python
//    std::wcout << L"��֤ Python ��װ..." << std::endl;
//    if (TestPython()) {
//        std::wcout << L"�ɹ���Python �ѿ���" << std::endl;
//
//        // ʾ�������� Python �ű�
//        system("python -c \"print('Hello from Python!')\"");
//    }
//    else {
//        std::wcerr << L"����Python ��֤ʧ��" << std::endl;
//        return 2;
//    }
//
//    // 5. ��ѡ����
//    // fs::remove_all(tempDir);
//
//    return 0;
//}