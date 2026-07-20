#pragma once


void LogOnContact(const ContactData& data) {

	SDL_Log("Contact %s between %s and %s", magic_enum::enum_name(data.phase).data(), data.self.name().c_str(), data.other.name().c_str());
	//SDL_Log("Contact Centroid : %f %f %f", data.centroid.GetX(), data.centroid.GetY(), data.centroid.GetZ());
}
