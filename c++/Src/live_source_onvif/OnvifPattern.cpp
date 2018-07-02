#include <memory>
#include "OnvifPattern.h"
#include "SHA1CodeArith.h"
#include "xstring.h"
#include "xchar.h"
#include "Byte.h"

std::string COnvifPattern::probe(const char* types, const char* scopes, const char* messageId /* = nullptr */) {
	return format("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
		"<Envelope xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\" xmlns=\"http://www.w3.org/2003/05/soap-envelope\">"
			"<Header>"
				"<wsa:MessageID xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">uuid:%s</wsa:MessageID>"
				"<wsa:To xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>"
				"<wsa:Action xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</wsa:Action>"
			"</Header>"
			"<Body>"
				"<Probe xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"
					"<Types>%s</Types>"
					"<Scopes>%s</Scopes>"
				"</Probe>"
			"</Body>"
		"</Envelope>", messageId == nullptr ? messageID().c_str() : messageId, types, scopes);
}

std::string COnvifPattern::getCapabilities(const char* username, const char* password, const char* category) {
	return format("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
		"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"
			"%s"
			"<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
				"<GetCapabilities xmlns=\"http://www.onvif.org/ver10/device/wsdl\">"
					"<Category>%s</Category>"
				"</GetCapabilities>"
			"</s:Body>"
		"</s:Envelope>", getSecurity(username, password).c_str(), category);
}

std::string COnvifPattern::getProfiles(const char* username, const char* password) {
	return format("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
		"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"
			"%s"
			"<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
				"<GetProfiles xmlns=\"http://www.onvif.org/ver10/media/wsdl\"/>"
			"</s:Body>"
		"</s:Envelope>", getSecurity(username, password).c_str());
}

std::string COnvifPattern::getStreamUri(const char* username, const char* password, const char* token, const char* protocol /* = "TCP" */) {
	return format("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
		"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"
			"%s"
			"<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
				"<GetStreamUri xmlns=\"http://www.onvif.org/ver10/media/wsdl\">"
					"<StreamSetup>"
						"<Stream xmlns=\"http://www.onvif.org/ver10/schema\">RTP-Unicast</Stream>"
						"<Transport xmlns=\"http://www.onvif.org/ver10/schema\">"
							"<Protocol>%s</Protocol>"
						"</Transport>"
					"</StreamSetup>"
					"<ProfileToken>%s</ProfileToken>"
				"</GetStreamUri>"
			"</s:Body>"
		"</s:Envelope>", getSecurity(username, password).c_str(), protocol, token);
}

std::string COnvifPattern::getPanTilt(const char* username, const char* password, const char* token, int x, int y) {
	return format("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
		"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"
			"%s"
			"<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
				"<ContinuousMove xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\">"
					"<ProfileToken>%s</ProfileToken>"
					"<Velocity>"
						"<PanTilt x=\"%d\" y=\"%d\" space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace\" xmlns=\"http://www.onvif.org/ver10/schema\" />"
					"</Velocity>"
				"</ContinuousMove>"
			"</s:Body>"
		"</s:Envelope>", getSecurity(username, password).c_str(), token, x, y);
}
std::string COnvifPattern::getZoom(const char* username, const char* password, const char* token, int x) {
	return format("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
		"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"
			"<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
				"<ContinuousMove xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\">"
					"<ProfileToken>%s</ProfileToken>"
					"<Velocity>"
						"<Zoom x=\"%d\" space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace\" xmlns=\"http://www.onvif.org/ver10/schema\" />"
					"</Velocity>"
				"</ContinuousMove>"
			"</s:Body>"
		"</s:Envelope>", getSecurity(username, password).c_str(), token, x);
}
std::string COnvifPattern::getStop(const char* username, const char* password, const char* token) {
	return format("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
		"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"
			"%s"
			"<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
				"<Stop xmlns=\"http://www.onvif.org/ver20/ptz/wsdl\">"
					"<ProfileToken>%s</ProfileToken>"
					"<PanTilt>true</PanTilt>"
					"<Zoom>true</Zoom>"
				"</Stop>"
			"</s:Body>"
		"</s:Envelope>", getSecurity(username, password).c_str(), token);
}

std::string COnvifPattern::format(const char* fmt, ...) {
	va_list argList;
	va_start(argList, fmt);
	int len = vsnprintf(nullptr, 0, fmt, argList) + 1;
	if (len > 0) {
		std::unique_ptr<char[]> buf(new char[len]);
		vsnprintf_s(buf.get(), len, len, fmt, argList);
		va_end(argList);
		return std::string(buf.get(), buf.get() + len - 1);
	}
	va_end(argList);
	return "";
}

std::string COnvifPattern::messageID(const char* prefix /* = "" */) {
	int r1, r2, r3, r4;
	static int k = 0xFACEB00C;
	int lo = k % 127773;
	int hi = k / 127773;
	r1 = (int)time(NULL);
	k = 16807 * lo - 2836 * hi;
	if (k <= 0) k += 0x7FFFFFFF;
	r2 = k;
	k &= 0x8FFFFFFF;
	r2 += rand();
	r3 = rand();
	r4 = rand();
	return format("%s%8.8x-%4.4hx-4%3.3hx-%4.4hx-%4.4hx%8.8x", prefix, r1, (short)(r2 >> 16), (short)(((short)r2 >> 4) & 0x0FFF), (short)(((short)(r3 >> 16) & 0x3FFF) | 0x8000), (short)r3, r4);
}

std::string COnvifPattern::getSecurity(const char* username, const char* password) {
	if (username == nullptr || strlen(username) < 1) return "";
	uint8_t rawNonce[16];
	time_t t = time(nullptr);
	srand(static_cast<int>(t));
	for (int i = 0; i < sizeof(rawNonce); i++) {
		rawNonce[i] = rand() % 255;
	}
	char created[100];
	size_t createdLen = strftime(created, 100, "%Y%m%dT%H%M%SZ", localtime(&t)); //format date and time. 
	std::string nonce = CByte::toBase64(rawNonce, 16).toString();
	created[createdLen] = '\0';
	
	CSHA1CodeArith sha1;
	sha1.Initialize();
	sha1.UpdateData(rawNonce, 16);
	sha1.UpdateData(reinterpret_cast<const uint8_t*>(created), static_cast<int>(createdLen));
	sha1.UpdateData(reinterpret_cast<const uint8_t*>(password), static_cast<int>(strlen(password)));
	unsigned char result[20];
	sha1.Finalize(result);
	std::string digest = CByte::toBase64(result, 20).toString();

	return format("<s:Header>"
		"<wsse:Security xmlns:wsse=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\" xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\">"
			"<wsse:UsernameToken>"
				"<wsse:Username>%s</wsse:Username>"
				"<wsse:Password Type=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest\">%s</wsse:Password>"
				"<wsse:Nonce>%s</wsse:Nonce>"
				"<wsu:Created>%s</wsu:Created>"
			"</wsse:UsernameToken>"
		"</wsse:Security>"
	"</s:Header>", username, digest.c_str(), nonce.c_str(), created);
}