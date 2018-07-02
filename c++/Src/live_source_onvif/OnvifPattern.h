#pragma once
#include <string>

class COnvifPattern {
public:
	static std::string probe(const char* types, const char* scopes = "", const char* messageId = nullptr);
	static std::string getCapabilities(const char* username, const char* password, const char* category);	//Media
	static std::string getProfiles(const char* username, const char* password);
	static std::string getStreamUri(const char* username, const char* password, const char* token, const char* protocol = "TCP");
	static std::string getPanTilt(const char* username, const char* password, const char* token, int x, int y);
	static std::string getZoom(const char* username, const char* password, const char* token, int x);
	static std::string getStop(const char* username, const char* password, const char* token);

public:
	static std::string messageID(const char* prefix = "");

private:
	static std::string format(const char* fmt, ...);
	static std::string getSecurity(const char* username, const char* password);
};

