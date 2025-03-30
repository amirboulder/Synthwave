#include <iostream>

#ifndef NDEBUG  // Code inside will only compile in debug mode

bool logToConsole = false;
bool run = false;



static int s_AllocationCount = 0;
static long s_AlocationTotalSize = 0;

size_t s_CurrentlyAllocated = 0;     // Track currently allocated memory

// Custom global delete operator
void operator delete(void* memory, size_t size) noexcept {
	if (memory == nullptr) return;

	// Update tracking variables
	s_AllocationCount--;
	s_AlocationTotalSize -= size;
	s_CurrentlyAllocated -= size;

	// Optional logging
	// cout << "Deallocated " << size << " bytes\n";
	//cout << "Allocation count: " << s_AllocationCount << '\n';
	if (logToConsole) {
		cout << "Currently allocated: " << s_CurrentlyAllocated << " bytes\n";
	}
	

	// Actually free the memory
	free(memory);
}

// No-size version of delete (required by C++ standard)
void operator delete(void* memory) noexcept {
	// Since we don't know the size, we can only decrement the allocation count
	if (memory == nullptr) return;

	s_AllocationCount--;
	// Note: Cannot update s_AlocationTotalSize or s_CurrentlyAllocated here
	// because we don't know the size

	// cout << "Deallocated unknown size\n";
	// cout << "Allocation count: " << s_AllocationCount << '\n';

	free(memory);
}

// Array versions of delete operators
void operator delete[](void* memory, size_t size) noexcept {
	operator delete(memory, size); // Reuse the same logic
}

void operator delete[](void* memory) noexcept {
	operator delete(memory); // Reuse the same logic
}

// Modified operator new to update currently allocated memory
void* operator new(size_t size) {
	s_AlocationTotalSize += size;
	s_AllocationCount++;
	s_CurrentlyAllocated += size;

	//cout << "Allocation count: " << s_AllocationCount << '\n';
    //cout << "Total allocation size: " << s_AlocationTotalSize << '\n';
	if (logToConsole) {
		cout << "Currently allocated: " << s_CurrentlyAllocated << " bytes\n";
	}

	return malloc(size);
}

// Array version of operator new
void* operator new[](size_t size) {
	return operator new(size); // Reuse the same logic
}


#endif