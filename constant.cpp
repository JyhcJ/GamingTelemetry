#include "constant.h"

 std::wstring g_domain;
 std::mutex g_domain_mutex;

// ∂¡»°
std::wstring get_g_domain() {
	std::lock_guard<std::mutex> lock(g_domain_mutex);
	return g_domain;
};

void set_g_domain(const std::wstring& domain) {
	std::lock_guard<std::mutex> lock(g_domain_mutex);
	g_domain = domain;
}