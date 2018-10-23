#include "TestRec.h"

TestRec::TestRec()
{
	this->clear();
}


TestRec::~TestRec()
{
}

TestRec::TestRec(const TestRec & other)
{
	
	this->id = other.id;
	this->name = other.name;

}

TestRec& TestRec::operator=(const TestRec & other)
{
	if (this == &other)
	{
		return *this;
	}

	this->id = other.id;
	this->name = other.name;

	return *this;

}

void TestRec::clear()
{
	this->id = 0;
	this->name = "";
}
