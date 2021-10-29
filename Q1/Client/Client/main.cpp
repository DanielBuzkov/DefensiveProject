#include <iostream>
#include "Client.h"

int main() {

	Client client;

	if (client.Init() == false) {
		std::cout << "Failed initializing client" << std::endl;
		return 1;
	}

	client.Run();

	return 0;
}
