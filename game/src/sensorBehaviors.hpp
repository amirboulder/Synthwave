#pragma once


void LogOnContactAdded(
	const ContactData& data) {

	SDL_Log("ContactAdded between %s and %s", data.self.name().c_str(), data.other.name().c_str());

}
