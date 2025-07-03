#include <ctime>
#include <iostream>
#include "config.h"

using namespace std;

int main(int argc, char *argv[])
{
	// Get calendar time and CPU time
	time_t time1, time2;
	clock_t startTime, endTime;
	float diff_time;
	time(&time1);
	startTime = clock();

	// Initialize a config parser
	Config config;
	for (int i = 1; i < argc; i++)
	{
		if (string(argv[i]) == "-c")
		{
			string config_file;
			config_file = string(argv[i + 1]);
			config.Parse(config_file);
			config.Run();
		}
		else if (string(argv[i]) == "-x")
		{
			config.Print();
		}
	}

	// Calculate spent calendar and CPU time
	endTime = clock();
	time(&time2);
	diff_time = difftime(time2, time1);
	cout << "Running(CPU) time: " << (double)(endTime - startTime) / CLOCKS_PER_SEC << " s." << endl;
	cout << "Actual time: " << diff_time << " s." << endl;
	return 0;
}
