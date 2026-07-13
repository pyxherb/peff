#include <cstdio>
#include <peff/utils/option.h>
#include <iostream>
#include <string>
#include <random>

struct Test {
	int i;

	void test() {
		printf("%i\n", i);
	}

	void test() const {
		printf("%i, const\n", i);
	}
};

int main() {
	Test a = { 123 };

	a.test();
	(static_cast<const Test &>(a)).test();

	peff::Option<Test &> x = a;

	x->test();

	peff::Option<const Test &> y = a;

	y->test();

	Test b = { 456 };

	(x = b)->test();

	(y = b)->test();

	return 0;
}
