/*
	$Id: PropertyConstructionTest1.cpp,v 1.3 2002/08/13 05:02:46 jrand Exp $
	
	This file implements the first test for the OpenBeOS BPropertyInfo code.
	It tests the Construction use cases.  It does so by doing the following:
	
	*/


#include "PropertyConstructionTest1.h"
#include <PropertyInfo.h>
#include <AppDefs.h>
#include <Message.h>
#include <TypeConstants.h>


/*
 *  Method:  PropertyConstructionTest1::PropertyConstructionTest1()
 *   Descr:  This is the constructor for this class.
 */
		

	PropertyConstructionTest1::PropertyConstructionTest1(std::string name) :
		TestCase(name)
{
	}


/*
 *  Method:  PropertyConstructionTest1::~PropertyConstructionTest1()
 *   Descr:  This is the destructor for this class.
 */
 

	PropertyConstructionTest1::~PropertyConstructionTest1()
{
	}
	
	
/*
 *  Method:  PropertyConstructionTest1::setUp()
 *   Descr:  This member function is called just prior to running the test.
 */


	void PropertyConstructionTest1::setUp(void)
{
	}


/*
 *  Method:  PropertyConstructionTest1::PerformTest()
 *   Descr:  This member function performs this test.
 */	


	void PropertyConstructionTest1::CheckProperty(
		BPropertyInfo *propTest,
	    property_info *prop_list,
	    value_info *value_list,
	    int32 prop_count,
	    int32 value_count,
	    ssize_t flat_size,
	    const char *flat_data)
{
	char buffer[512];
	
	assert(propTest->Properties() == prop_list);
	assert(propTest->Values() == value_list);
	assert(propTest->CountProperties() == prop_count);
	assert(propTest->CountValues() == value_count);
	assert(!propTest->IsFixedSize());
	assert(propTest->TypeCode() == B_PROPERTY_INFO_TYPE);
	assert(propTest->AllowsTypeCode(B_PROPERTY_INFO_TYPE));
	assert(!propTest->AllowsTypeCode(B_TIME_TYPE));
	assert(propTest->FlattenedSize() == flat_size);
	assert(propTest->Flatten(buffer, sizeof(buffer)/ sizeof(buffer[0])) == B_OK);
	assert(memcmp(buffer, flat_data, propTest->FlattenedSize()) == 0);
	}
	
	
/*
 *  Method:  PropertyConstructionTest1::PerformTest()
 *   Descr:  This member function performs this test.
 */	


	void PropertyConstructionTest1::PerformTest(void)
{
	struct property_info prop1[] = { 0 };
	char flatten1[] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0 };
	struct property_info prop2[] = {
		{ "duck", {B_GET_PROPERTY, B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, B_INDEX_SPECIFIER, 0}, "get or set duck"}, 
    	{ "head", {B_GET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "get head"}, 
        { "head", {B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "set head"}, 
        { "feet", {0}, {0}, "can do anything with his orange feet"}, 
        0 // terminate list 
    };
	struct property_info *prop_lists[] = { NULL, prop1, prop2 };
	int32 prop_counts[] = { 0, 0, 4 };
	ssize_t prop_size[] = { 9, 9, 212 };
	
	struct value_info value1[] = { 0 };
	struct value_info value2[] = {
		{ "Value1", 5, B_COMMAND_KIND, "This is the usage" },
		{ "Value2", 6, B_TYPE_CODE_KIND, "This is the usage" },
		0
	};
	struct value_info *value_lists[] = { NULL, value1, value2 };
	int32 value_counts[] = { 0, 0, 2 };
	ssize_t value_size[] = { 0, 2, 76 };
	char flattened_data[3][3][300] = {
		{
			{0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0},
			{0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0},
			{0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0,
			 0x0, 0x0, 0x5, 0x0, 0x0, 0x0, 0x56, 0x61, 0x6c, 0x75, 0x65, 0x31,
			 0x0, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
			 0x65, 0x20, 0x75, 0x73, 0x61, 0x67, 0x65, 0x0, 0x0, 0x0, 0x0,
			 0x0, 0x1, 0x0, 0x0, 0x0, 0x6, 0x0, 0x0, 0x0, 0x56, 0x61, 0x6c,
			 0x75, 0x65, 0x32, 0x0, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73,
			 0x20, 0x74, 0x68, 0x65, 0x20, 0x75, 0x73, 0x61, 0x67, 0x65, 0x0,
			 0x0, 0x0, 0x0, 0x0}
			},
		{
			{0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0},
			{0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0},
			{0x0, 0x0, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0,
			 0x0, 0x0, 0x5, 0x0, 0x0, 0x0, 0x56, 0x61, 0x6c, 0x75, 0x65, 0x31,
			 0x0, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
			 0x65, 0x20, 0x75, 0x73, 0x61, 0x67, 0x65, 0x0, 0x0, 0x0, 0x0,
			 0x0, 0x1, 0x0, 0x0, 0x0, 0x6, 0x0, 0x0, 0x0, 0x56, 0x61, 0x6c,
			 0x75, 0x65, 0x32, 0x0, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73,
			 0x20, 0x74, 0x68, 0x65, 0x20, 0x75, 0x73, 0x61, 0x67, 0x65, 0x0,
			 0x0, 0x0, 0x0, 0x0}
			},
		{
			{0x0, 0x4, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x64, 0x75, 0x63,
			 0x6b, 0x0, 0x67, 0x65, 0x74, 0x20, 0x6f, 0x72, 0x20, 0x73, 0x65,
			 0x74, 0x20, 0x64, 0x75, 0x63, 0x6b, 0x0, 0x0, 0x0, 0x0, 0x0, 0x54,
			 0x45, 0x47, 0x50, 0x54, 0x45, 0x53, 0x50, 0x0, 0x0, 0x0, 0x0, 0x1,
			 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x68, 0x65,
			 0x61, 0x64, 0x0, 0x67, 0x65, 0x74, 0x20, 0x68, 0x65, 0x61, 0x64,
			 0x0, 0x0, 0x0, 0x0, 0x0, 0x54, 0x45, 0x47, 0x50, 0x0, 0x0, 0x0,
			 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x68, 0x65, 0x61,
			 0x64, 0x0, 0x73, 0x65, 0x74, 0x20, 0x68, 0x65, 0x61, 0x64, 0x0,
			 0x0, 0x0, 0x0, 0x0, 0x54, 0x45, 0x53, 0x50, 0x0, 0x0, 0x0, 0x0,
			 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x66, 0x65, 0x65, 0x74, 
			 0x0, 0x63, 0x61, 0x6e, 0x20, 0x64, 0x6f, 0x20, 0x61, 0x6e, 0x79,
			 0x74, 0x68, 0x69, 0x6e, 0x67, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20,
			 0x68, 0x69, 0x73, 0x20, 0x6f, 0x72, 0x61, 0x6e, 0x67, 0x65, 0x20,
			 0x66, 0x65, 0x65, 0x74, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
			{0x0, 0x4, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x0, 0x64, 0x75, 0x63,
			 0x6b, 0x0, 0x67, 0x65, 0x74, 0x20, 0x6f, 0x72, 0x20, 0x73, 0x65,
			 0x74, 0x20, 0x64, 0x75, 0x63, 0x6b, 0x0, 0x0, 0x0, 0x0, 0x0, 0x54,
			 0x45, 0x47, 0x50, 0x54, 0x45, 0x53, 0x50, 0x0, 0x0, 0x0, 0x0, 0x1,
			 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x68, 0x65,
			 0x61, 0x64, 0x0, 0x67, 0x65, 0x74, 0x20, 0x68, 0x65, 0x61, 0x64,
			 0x0, 0x0, 0x0, 0x0, 0x0, 0x54, 0x45, 0x47, 0x50, 0x0, 0x0, 0x0,
			 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x68, 0x65, 0x61,
			 0x64, 0x0, 0x73, 0x65, 0x74, 0x20, 0x68, 0x65, 0x61, 0x64, 0x0,
			 0x0, 0x0, 0x0, 0x0, 0x54, 0x45, 0x53, 0x50, 0x0, 0x0, 0x0, 0x0,
			 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x66, 0x65, 0x65, 0x74,
			 0x0, 0x63, 0x61, 0x6e, 0x20, 0x64, 0x6f, 0x20, 0x61, 0x6e, 0x79,
			 0x74, 0x68, 0x69, 0x6e, 0x67, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20,
			 0x68, 0x69, 0x73, 0x20, 0x6f, 0x72, 0x61, 0x6e, 0x67, 0x65, 0x20,
			 0x66, 0x65, 0x65, 0x74, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
			{0x0, 0x4, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x0, 0x64, 0x75, 0x63,
			 0x6b, 0x0, 0x67, 0x65, 0x74, 0x20, 0x6f, 0x72, 0x20, 0x73, 0x65,
			 0x74, 0x20, 0x64, 0x75, 0x63, 0x6b, 0x0, 0x0, 0x0, 0x0, 0x0, 0x54,
			 0x45, 0x47, 0x50, 0x54, 0x45, 0x53, 0x50, 0x0, 0x0, 0x0, 0x0, 0x1,
			 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x68, 0x65,
			 0x61, 0x64, 0x0, 0x67, 0x65, 0x74, 0x20, 0x68, 0x65, 0x61, 0x64,
			 0x0, 0x0, 0x0, 0x0, 0x0, 0x54, 0x45, 0x47, 0x50, 0x0, 0x0, 0x0,
			 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x68, 0x65, 0x61,
			 0x64, 0x0, 0x73, 0x65, 0x74, 0x20, 0x68, 0x65, 0x61, 0x64, 0x0,
			 0x0, 0x0, 0x0, 0x0, 0x54, 0x45, 0x53, 0x50, 0x0, 0x0, 0x0, 0x0,
			 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x66, 0x65, 0x65, 0x74,
			 0x0, 0x63, 0x61, 0x6e, 0x20, 0x64, 0x6f, 0x20, 0x61, 0x6e, 0x79,
			 0x74, 0x68, 0x69, 0x6e, 0x67, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20,
			 0x68, 0x69, 0x73, 0x20, 0x6f, 0x72, 0x61, 0x6e, 0x67, 0x65, 0x20,
			 0x66, 0x65, 0x65, 0x74, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x0,
			 0x0, 0x0, 0x0, 0x0, 0x5, 0x0, 0x0, 0x0, 0x56, 0x61, 0x6c, 0x75,
			 0x65, 0x31, 0x0, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20,
			 0x74, 0x68, 0x65, 0x20, 0x75, 0x73, 0x61, 0x67, 0x65, 0x0, 0x0,
			 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x6, 0x0, 0x0, 0x0, 0x56, 0x61,
			 0x6c, 0x75, 0x65, 0x32, 0x0, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69,
			 0x73, 0x20, 0x74, 0x68, 0x65, 0x20, 0x75, 0x73, 0x61, 0x67, 0x65,
			 0x0, 0x0, 0x0, 0x0, 0x0}
		}
	};
	int i, j;
	
	BPropertyInfo *propTest = new BPropertyInfo;
	CheckProperty(propTest, NULL, NULL, 0, 0, 9, flattened_data[0][0]);
	delete propTest;

	for (i=0; i < sizeof(prop_counts) / sizeof(int); i++) {
		propTest = new BPropertyInfo(prop_lists[i]);
		CheckProperty(propTest, prop_lists[i], NULL, prop_counts[i], 0,
				      prop_size[i], flattened_data[i][0]);
		delete propTest;
	
		for (j=0; j < sizeof(value_counts) / sizeof(int); j++) {
			propTest = new BPropertyInfo(prop_lists[i], value_lists[j]);
			CheckProperty(propTest, prop_lists[i], value_lists[j],
			              prop_counts[i], value_counts[j],
			              prop_size[i] + value_size[j], flattened_data[i][j]);
			delete propTest;
			
			propTest = new BPropertyInfo(prop_lists[i], value_lists[j], false);
			CheckProperty(propTest, prop_lists[i], value_lists[j],
			              prop_counts[i], value_counts[j],
			              prop_size[i] + value_size[j], flattened_data[i][j]);
			delete propTest;
		}
	}
}


/*
 *  Method:  PropertyConstructionTest1::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "PropertyConstructionTest1".  The test
 *           is created as a ThreadedTestCase (typedef'd as
 *           PropertyConstructionTest1Caller) with only one thread.
 */

 Test *PropertyConstructionTest1::suite(void)
{	
	typedef CppUnit::TestCaller<PropertyConstructionTest1>
		PropertyConstructionTest1Caller;
		
	return(new PropertyConstructionTest1Caller("BPropertyInfo::Construction Test", &PropertyConstructionTest1::PerformTest));
	}
 

