Package: boost-cmake:x64-windows@1.88.0

**Host Environment**

- Host: x64-windows
- Compiler: MSVC 19.44.35209.0
-    vcpkg-tool version: 2025-06-02-145689e84b7637525510e2c9b4ee603fda046b56
    vcpkg-scripts version: 984f9232b2 2025-06-06 (7 hours ago)

**To Reproduce**

`vcpkg install `

**Failure logs**

```
-- Using cached boostorg-cmake-boost-1.88.0.tar.gz
-- Cleaning sources at D:/vcpkg/buildtrees/boost-cmake/src/ost-1.88.0-7ca645f160.clean. Use --editable to skip cleaning for the packages you specify.
-- Extracting source D:/vcpkg/downloads/boostorg-cmake-boost-1.88.0.tar.gz
-- Applying patch 0001-vcpkg-build.patch
-- Applying patch 0002-remove-prefix-and-suffix.patch
-- Using source at D:/vcpkg/buildtrees/boost-cmake/src/ost-1.88.0-7ca645f160.clean
-- Found external ninja('1.12.1').
-- Configuring x64-windows
-- Building x64-windows-dbg
-- Building x64-windows-rel
-- Up-to-date: D:/vcpkg/packages/boost-cmake_x64-windows/share/boost/cmake-build
-- Installing: D:/vcpkg/packages/boost-cmake_x64-windows/share/boost/cmake-build/BoostFetch.cmake
-- Installing: D:/vcpkg/packages/boost-cmake_x64-windows/share/boost/cmake-build/BoostInstall.cmake
-- Installing: D:/vcpkg/packages/boost-cmake_x64-windows/share/boost/cmake-build/BoostMessage.cmake
-- Installing: D:/vcpkg/packages/boost-cmake_x64-windows/share/boost/cmake-build/BoostRoot.cmake
-- Installing: D:/vcpkg/packages/boost-cmake_x64-windows/share/boost/cmake-build/BoostTest.cmake
-- Installing: D:/vcpkg/packages/boost-cmake_x64-windows/share/boost/cmake-build/BoostTestJamfile.cmake
-- Installing: D:/vcpkg/packages/boost-cmake_x64-windows/share/boost-cmake/vcpkg-port-config.cmake
-- Installing: D:/vcpkg/packages/boost-cmake_x64-windows/share/boost-cmake/usage
Downloading https://raw.githubusercontent.com/boostorg/boost/refs/tags/boost-1.88.0/LICENSE_1_0.txt -> boost-1.88.0-LICENSE_1_0.txt
warning: Download https://raw.githubusercontent.com/boostorg/boost/refs/tags/boost-1.88.0/LICENSE_1_0.txt failed -- retrying after 1000ms
warning: Download https://raw.githubusercontent.com/boostorg/boost/refs/tags/boost-1.88.0/LICENSE_1_0.txt failed -- retrying after 2000ms
warning: Download https://raw.githubusercontent.com/boostorg/boost/refs/tags/boost-1.88.0/LICENSE_1_0.txt failed -- retrying after 4000ms
error: https://raw.githubusercontent.com/boostorg/boost/refs/tags/boost-1.88.0/LICENSE_1_0.txt: WinHttpSendRequest failed with exit code 12002. 操作超时
note: If you are using a proxy, please ensure your proxy settings are correct.
Possible causes are:
1. You are actually using an HTTP proxy, but setting HTTPS_PROXY variable to `https//address:port`.
This is not correct, because `https://` prefix claims the proxy is an HTTPS proxy, while your proxy (v2ray, shadowsocksr, etc...) is an HTTP proxy.
Try setting `http://address:port` to both HTTP_PROXY and HTTPS_PROXY instead.
2. If you are using Windows, vcpkg will automatically use your Windows IE Proxy Settings set by your proxy software. See: https://github.com/microsoft/vcpkg-tool/pull/77
The value set by your proxy might be wrong, or have same `https://` prefix issue.
3. Your proxy's remote server is our of service.
If you believe this is not a temporary download server failure and vcpkg needs to be changed to download this file from a different location, please submit an issue to https://github.com/Microsoft/vcpkg/issues
CMake Error at scripts/cmake/vcpkg_download_distfile.cmake:136 (message):
  Download failed, halting portfile.
Call Stack (most recent call first):
  ports/boost-cmake/portfile.cmake:27 (vcpkg_download_distfile)
  scripts/ports.cmake:206 (include)



```

**Additional context**

<details><summary>vcpkg.json</summary>

```
{
  "dependencies": [
    "nlohmann-json",
    "fmt",
    "spdlog",
    "boost-algorithm"
  ]
}

```
</details>
