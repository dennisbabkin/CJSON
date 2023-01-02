//Class that implements JSON parser
//
// (C) 2015, www.dennisbabkin.com
//

#pragma once

#include <string>
#include <vector>


#ifdef _WIN32
//Windows specific includes
#include <Windows.h>
#include <tchar.h>

#elif __APPLE__
//macOS specific
#include <CoreFoundation/CFString.h>

#endif


#include <algorithm>
#include <functional>
#include <cctype>

#include <assert.h>


#ifndef SIZEOF
#define SIZEOF(f) (sizeof(f) / sizeof(f[0]))
#endif

#ifndef TSIZEOF
#define TSIZEOF(f) (SIZEOF(f) - 1)
#endif

#ifndef _DEBUG
#define _DEBUG DEBUG
#endif

#ifndef ASSERT
#define ASSERT(f) assert(f)
#endif

#ifndef VERIFY
#ifdef _DEBUG
#define VERIFY(f) ASSERT(f)
#else
#define VERIFY(f) (f)
#endif
#endif



namespace json
{


#ifdef _WIN32
//Windows specific
//---------------------------------------------------
#define std_wstring std::wstring

#define STRLEN(s) wcslen(s)

#define L(txt) L##txt



#elif __APPLE__
//macOS specific
//---------------------------------------------------
#define std_wstring std::string

#define STRLEN(s) strlen(s)

#define WCHAR char
#define LPCTSTR const char*
#define UINT unsigned int
#define BYTE unsigned char

#define NO_ERROR                    0
#define ERROR_INVALID_PARAMETER     EINVAL
#define ERROR_HANDLE_EOF            EDQUOT
#define ERROR_INVALID_DATA          EBADF
#define ERROR_OUTOFMEMORY           ENOMEM
#define ERROR_BAD_FORMAT            ENOEXEC

#define L(txt) txt

#else
//Unsupported OS
#error your_os_is_not_supported
#endif



#define UTF8_MAX_VAL 0x001FFFFF         //Maximum allowed utf-8 value (inclusive)




enum JSON_VALUE_TYPE
{
	JVT_NONE,							//Not filled yet
	JVT_PLAIN,							// 25, 167.6, 12E40, -12, +12, true, false, null
	JVT_DOUBLE_QUOTED,					// "string"
	JVT_ARRAY,							// [ val1, val2 ]
	JVT_OBJECT,							// { "name1":"value1", "name2" : "value2" }
};

struct JSON_VALUE
{
	JSON_VALUE_TYPE valType;			//Type of value

    std_wstring strValue;				//Used in case of: JVT_PLAIN, JVT_DOUBLE_QUOTED
	void* pValue;						//Pointer to either JSON_OBJECT or JSON_ARRAY, otherwise nullptr

	JSON_VALUE()
	{
		valType = JVT_NONE;
		pValue = nullptr;
	}

	bool isEmptyValue()
	{
		//RETURN: = true if this value wasn't filled yet
#ifdef _DEBUG
		if(valType == JVT_NONE)
        {
            ASSERT(pValue == nullptr);
        }
#endif
		return valType == JVT_NONE;
	}
};

struct JSON_OBJECT_ELEMENT
{
    std_wstring strName;
	JSON_VALUE val;
};

struct JSON_ARRAY_ELEMENT
{
	JSON_VALUE val;
};


struct JSON_ERROR
{
	intptr_t nErrIndex;				//Index of WCHAR in the original JSON string where the error was triggered (or -1 if not known)
    std_wstring strErrDesc;		    //English description of the error

	JSON_ERROR()
	{
		nErrIndex = -1;
		bFilled = false;
	}

	LPCTSTR getErrorLocation(std_wstring& strJSON)
	{
		//'strJSON' = original JSON that was parsed
		//RETURN:
		//		= Pointer to the location of the error in 'strJSON', or
		//		= nullptr if error
		return getErrorLocation(strJSON.c_str(), (int)strJSON.size());
	}

	LPCTSTR getErrorLocation(LPCTSTR pStrJSON, int nchLenJSON)
	{
		//'pStrJSON' = original JSON that was parsed
		//'nchLenJSON' = length of 'pStrJSON' in TCHARs
		//RETURN:
		//		= Pointer to the location of the error in 'pStrJSON', or
		//		= nullptr if error
		if(pStrJSON &&
			!isEmpty() &&
			nErrIndex >= 0 &&
			nErrIndex <= nchLenJSON)
		{
			return pStrJSON + nErrIndex;
		}

		return nullptr;
	}

	bool isEmpty()
	{
		return bFilled ? false : true;
	}

	void markFilled()
	{
		bFilled = true;
	}

private:
    bool bFilled;
};


struct JSON_OBJECT
{
	std::vector<JSON_OBJECT_ELEMENT> arrObjElmts;

	JSON_OBJECT()
	{
	}

private:
	//No assignment or copy constructor
	JSON_OBJECT(const JSON_OBJECT& s) = delete;
	JSON_OBJECT& operator = (const JSON_OBJECT& s) = delete;
};



struct JSON_ARRAY
{
	std::vector<JSON_ARRAY_ELEMENT> arrArrElmts;

	JSON_ARRAY()
	{
	}

private:
	//No assignment or copy constructor
	JSON_ARRAY(const JSON_ARRAY& s) = delete;
	JSON_ARRAY& operator = (const JSON_ARRAY& s) = delete;
};



struct JSON_SRCH
{
	intptr_t nIndex;			//[Used internally]

	JSON_SRCH()
	{
		nIndex = 0;
	}

    intptr_t getIndexFoundAt()
	{
		//RETURN:
		//		= [0 and up) Index found at, or
		//		= -1 if none
		if(nIndex > 0)
			return nIndex - 1;
		else
			return -1;
	}
};






enum JSON_NODE_TYPE
{
	JNT_ERROR = -1,			//Error
	JNT_NONE = 0,			//Not known

	JNT_ROOT = 1,			//Root node
	JNT_NULL = 2,			//null
	JNT_BOOLEAN = 3,		//true or false
	JNT_INTEGER = 4,		//integer
	JNT_FLOAT = 5,			//floating point number
	JNT_STRING = 6,			//"string" - Note that "123" will also be returned as string, as well as "true" will be returned as string (because of double quotes)
	JNT_ARRAY = 7,			//[1, 2, "3s"]
	JNT_OBJECT = 8,			//{"name": "value"}
};


struct JSON_DATA;

struct JSON_NODE
{
	JSON_NODE_TYPE typeNode;
    std_wstring strName;
	JSON_VALUE* pVal;

	//extern struct JSON_DATA;
	JSON_DATA* pJSONData;				//Pointer to original JSON data, where this node is part of

	JSON_NODE()
	{
		//Creates an empty node
		//INFO: It can be used for:
		//		 - Search via CJSON::FindNodeBy*()
		//		 - to call JSON_NODE::setAsRootNode() on
		//		 - to call JSON_DATA::getRootNode() on
		typeNode = JNT_NONE;
		pVal = nullptr;
		pJSONData = nullptr;
	}

	JSON_NODE(JSON_DATA* pJSON_Data, LPCTSTR pStrName = nullptr, JSON_NODE_TYPE type = JNT_OBJECT)
	{
		//Creates a root node
		//INFO: It can be used for:
		//		 - Adding other nodes via JSON_NODE::addNode*()
		//'pJSON_Data' = must be provided to create a root node (otherwise creates an empty node)
		//'pStrName' = name for the node if we'll be adding it to other nodes later
		//'type' = type of node to create, can be: JNT_OBJECT, JNT_ARRAY or JNT_ROOT

		//Empty node
		typeNode = JNT_NONE;
		pVal = nullptr;
		pJSONData = nullptr;

		if(pJSON_Data)
		{
			switch(type)
			{
			case JNT_OBJECT:
			case JNT_ARRAY:
				{
					//Set as object or array
					VERIFY(setAsEmptyNode(pJSON_Data, type));
				}
				break;
			case JNT_ROOT:
				{
					//Set as root
					VERIFY(setAsRootNode(pJSON_Data));
				}
				break;
			default:
				//Wrong type
				ASSERT(nullptr);
				break;
			}
		}

		//Set node name
		strName = pStrName ? pStrName : L("");
	}

    bool isNodeSet()
	{
		//RETURN: = true if this node is set to point to JSON node
		return (pVal && pJSONData) ? true : false;
	}

	JSON_NODE_TYPE getNodeType(bool bAllowRootType = false)
	{
		//'bAllowRootType' = set to false to retrieve type for the root node
		//RETURN: = Type of this node
		JSON_NODE_TYPE type = typeNode;
		if(type == JNT_ROOT &&
			!bAllowRootType)
		{
			//Determine it
			type = _determineNodeType(pVal);
		}

		return type;
	}

    bool getValueAsString(std_wstring* pOutStr)
	{
		//RETURN: = true if value was available
        std_wstring str;
        bool bRes = false;

		if(isNodeSet())
		{
			ASSERT(pVal);
			switch(pVal->valType)
			{
			case JVT_PLAIN:
			case JVT_DOUBLE_QUOTED:
				{
					str = pVal->strValue;
					bRes = true;
				}
				break;

			case JVT_ARRAY:
				{
					str = L("[Array]");
					bRes = true;
				}
				break;

			case JVT_OBJECT:
				{
					str = L("[Object]");
					bRes = true;
				}
				break;
                    
            default:
                break;
			}
		}

		if(pOutStr)
			*pOutStr = str;

		return bRes;
	}

    bool getValueAsInt32(int* pOutVal, bool bCaseSensitive = true)
	{
		//'bCaseSensitive' = true to look only for "true" or "false" in case sensitive way
		//'pOutVal' = if not nullptr, receives the int (in case of an overflow it will be set to largest available integer and return value will be false)
		//RETURN: = true if value was available & valid
		int nVal = 0;
        bool bRes = false;

		//Read it as 64-bit int
        int64_t iiVal = 0;
		if(getValueAsInt64(&iiVal, bCaseSensitive))
		{
			//Is this number within our range
			if(iiVal >= INT_MIN)
			{
				if(iiVal <= INT_MAX)
				{
					bRes = true;
					nVal = (int)iiVal;
				}
				else
				{
					//Overflow
					ASSERT(nullptr);
					nVal = INT_MAX;
				}
			}
			else
			{
				//Overflow
				ASSERT(nullptr);
				nVal = INT_MIN;
			}
		}

		if(pOutVal)
			*pOutVal = nVal;

		return bRes;
	}

    bool getValueAsInt64(int64_t* pOutVal, bool bCaseSensitive = true)
	{
		//'bCaseSensitive' = true to look only for "true" or "false" in case sensitive way
		//RETURN: = true if value was available
        int64_t iiVal = 0;
        bool bRes = false;

		if(isNodeSet())
		{
			ASSERT(pVal);
			if(pVal->valType == JVT_PLAIN ||
				pVal->valType == JVT_DOUBLE_QUOTED)
			{
				//Check type as well
				if(typeNode == JNT_INTEGER)
				{
#ifdef _WIN32
                    //Windows-specific
					iiVal = _ttoi64(pVal->strValue.c_str());
#elif __APPLE__
                    //macOS specific
                    char* pEnd = nullptr;
                    iiVal = strtoull(pVal->strValue.c_str(), &pEnd, 10);
#endif
                    
					bRes = true;
				}
				else if(typeNode == JNT_BOOLEAN)
				{
					bool bBool = false;
					if(getValueAsBool(&bBool, bCaseSensitive))
					{
						iiVal = bBool ? 1 : 0;
						bRes = true;
					}
				}
				else if(typeNode == JNT_FLOAT)
				{
					double fVal = 0.0;
					if(parseFloat(pVal->strValue.c_str(), &fVal))
					{
						iiVal = (int64_t)(fVal + 0.5);		//Round it to the nearest integer
						bRes = true;
					}
				}
				else if(typeNode == JNT_STRING)
				{
					if(isIntegerBase10String(pVal->strValue.c_str()))
					{
#ifdef _WIN32
                        //Windows-specific
						iiVal = _ttoi64(pVal->strValue.c_str());
#elif __APPLE__
                        //macOS specific
                        char* pEnd = nullptr;
                        iiVal = strtoull(pVal->strValue.c_str(), &pEnd, 10);
#endif
                        bRes = true;
					}
					else
					{
						double fVal = 0.0;
						if(parseFloat(pVal->strValue.c_str(), &fVal))
						{
							iiVal = (int64_t)(fVal + 0.5);		//Round it to the nearest integer
							bRes = true;
						}
					}
				}
			}
		}

		if(pOutVal)
			*pOutVal = iiVal;

		return bRes;
	}

    bool getValueAsBool(bool* pOutBool, bool bCaseSensitive = true)
	{
		//'bCaseSensitive' = true to look only for "true" or "false" in case sensitive way
		//RETURN: = true if value was available
		bool bVal = false;
        bool bRes = false;

		if(isNodeSet())
		{
			ASSERT(pVal);
			if(pVal->valType == JVT_PLAIN ||
				pVal->valType == JVT_DOUBLE_QUOTED)
			{
				//Convert
				bool bCaseSens = !!bCaseSensitive;

				if(JSON_NODE::compareStringsEqual(pVal->strValue, L("true"), bCaseSens))
				{
					bVal = true;
					bRes = true;
				}
				else if(JSON_NODE::compareStringsEqual(pVal->strValue, L("false"), bCaseSens))
				{
					bVal = false;
					bRes = true;
				}
			}
		}

		if(pOutBool)
			*pOutBool = bVal;

		return bRes;
	}

    bool isNullValue(bool bCaseSensitive = true)
	{
		//Checks if node value is set to "null"
		//'bCaseSensitive' = true to look only for "null" in case sensitive way
		//RETURN:
		//		= true if yes, it is set to "null"
        bool bRes = false;

		if(isNodeSet())
		{
			ASSERT(pVal);
			if(pVal->valType == JVT_PLAIN ||
				pVal->valType == JVT_DOUBLE_QUOTED)
			{
				if(JSON_NODE::compareStringsEqual(pVal->strValue, L("null"), !!bCaseSensitive))
				{
					bRes = true;
				}
			}
		}

		return bRes;
	}


	intptr_t getNodeCount()
	{
		//RETURN:
		//		= [0 and up) Number of nodes in this node, or
		//		= -1 if it's not an array or an object node
		intptr_t nCnt = -1;

		if(isNodeSet())
		{
			ASSERT(pVal);
			if(pVal->valType == JVT_ARRAY)
			{
				JSON_ARRAY* pJA = (JSON_ARRAY*)pVal->pValue;
				ASSERT(pJA);
				if(pJA)
				{
					nCnt = pJA->arrArrElmts.size();
				}
			}
			else if(pVal->valType == JVT_OBJECT)
			{
				JSON_OBJECT* pJO = (JSON_OBJECT*)pVal->pValue;
				ASSERT(pJO);
				if(pJO)
				{
					nCnt = pJO->arrObjElmts.size();
				}
			}
		}

		return nCnt;
	}


	JSON_NODE_TYPE findNodeByIndex(intptr_t nIndex, JSON_NODE* pJNodeFound = nullptr);
	JSON_NODE_TYPE findNodeByIndexAndGetValueAsString(intptr_t nIndex, std_wstring* pOutStr = nullptr);
	JSON_NODE_TYPE findNodeByName(LPCTSTR pStrName, JSON_NODE* pJNodeFound, bool bCaseSensitive = false, JSON_SRCH* pJSrch = nullptr);
	JSON_NODE_TYPE findNodeByNameAndGetValueAsString(LPCTSTR pStrName, std_wstring* pOutStr = nullptr, bool bCaseSensitive = false);
    JSON_NODE_TYPE findNodeByNameAndGetValueAsInt32(LPCTSTR pStrName, int* pOuVal = nullptr, bool bCaseSensitive = false);
    JSON_NODE_TYPE findNodeByNameAndGetValueAsInt64(LPCTSTR pStrName, int64_t* pOuVal = nullptr, bool bCaseSensitive = false);
    JSON_NODE_TYPE findNodeByNameAndGetValueAsBool(LPCTSTR pStrName, bool* pOuVal = nullptr, bool bCaseSensitive = false);
    bool setAsRootNode(JSON_DATA* pJSON_Data = nullptr);
    bool setAsEmptyNode(JSON_DATA* pJSON_Data = nullptr, JSON_NODE_TYPE type = JNT_OBJECT);
    bool addNode(JSON_NODE* pJNode);
    bool addNode_String(LPCTSTR pStrName, LPCTSTR pStrValue = nullptr);
    bool addNode_BOOL(LPCTSTR pStrName, bool bValue);
    bool addNode_Bool(LPCTSTR pStrName, bool bValue);
    bool addNode_Null(LPCTSTR pStrName);
    bool addNode_Int(LPCTSTR pStrName, LPCTSTR pStrValue);
    bool addNode_Int(LPCTSTR pStrName, int nValue);
    bool addNode_Int64(LPCTSTR pStrName, LPCTSTR pStrValue);
    bool addNode_Int64(LPCTSTR pStrName, int64_t iiValue);
    bool addNode_Double(LPCTSTR pStrName, LPCTSTR pStrValue);
    bool addNode_Double(LPCTSTR pStrName, double fValue);

    intptr_t setNodeByName(LPCTSTR pStrName, JSON_NODE* pJNode, bool bCaseSensitive = false);
    intptr_t setNodeByName_String(LPCTSTR pStrName, LPCTSTR pStrValue, bool bCaseSensitive = false);
    intptr_t setNodeByName_BOOL(LPCTSTR pStrName, bool bValue, bool bCaseSensitive = false);
    intptr_t setNodeByName_Bool(LPCTSTR pStrName, bool bValue, bool bCaseSensitive = false);
    intptr_t setNodeByName_Null(LPCTSTR pStrName, bool bCaseSensitive = false);
    intptr_t setNodeByName_Int(LPCTSTR pStrName, LPCTSTR pStrValue, bool bCaseSensitive = false);
    intptr_t setNodeByName_Int(LPCTSTR pStrName, int nValue, bool bCaseSensitive = false);
    intptr_t setNodeByName_Int64(LPCTSTR pStrName, LPCTSTR pStrValue, bool bCaseSensitive = false);
    intptr_t setNodeByName_Int64(LPCTSTR pStrName, int64_t iiValue, bool bCaseSensitive = false);
    intptr_t setNodeByName_Double(LPCTSTR pStrName, LPCTSTR pStrValue, bool bCaseSensitive = false);
    intptr_t setNodeByName_Double(LPCTSTR pStrName, double fValue, bool bCaseSensitive = false);

    bool setNodeByIndex(intptr_t nIndex, JSON_NODE* pJNode);
    bool setNodeByIndex_String(intptr_t nIndex, LPCTSTR pStrValue);
    bool setNodeByIndex_BOOL(intptr_t nIndex, bool bValue);
    bool setNodeByIndex_Bool(intptr_t nIndex, bool bValue);
    bool setNodeByIndex_Null(intptr_t nIndex);
    bool setNodeByIndex_Int(intptr_t nIndex, LPCTSTR pStrValue);
    bool setNodeByIndex_Int(intptr_t nIndex, int nValue);
    bool setNodeByIndex_Int64(intptr_t nIndex, LPCTSTR pStrValue);
    bool setNodeByIndex_Int64(intptr_t nIndex, int64_t iiValue);
    bool setNodeByIndex_Double(intptr_t nIndex, LPCTSTR pStrValue);
    bool setNodeByIndex_Double(intptr_t nIndex, double fValue);

    intptr_t removeNodeByName(LPCTSTR pStrName, bool bCaseSensitive = false);
    bool removeNodeByIndex(intptr_t nIndex);

	static bool compareStringsEqual(LPCTSTR pStr1, LPCTSTR pStr2, bool bCaseSensitive);
	static bool compareStringsEqual(LPCTSTR pStr1, intptr_t nchLn1, LPCTSTR pStr2, intptr_t nchLn2, bool bCaseSensitive);
	static bool compareStringsEqual(std_wstring& str1, std_wstring& str2, bool bCaseSensitive);
	static bool compareStringsEqual(std_wstring& str1, LPCTSTR pStr2, bool bCaseSensitive);
    
#ifdef __APPLE__
    //macOS specific

    static intptr_t getUtf8Char(const char* pStr,
                                intptr_t i,
                                intptr_t nLn,
                                unsigned int* pOutChar = nullptr);

    static bool appendUtf8Char(std_wstring& str,
                                unsigned int z);
    
#endif
    
    
private:
    bool _addNode_WithType(LPCTSTR pStrName, JSON_VALUE_TYPE type, LPCTSTR pStrValue);
    intptr_t _setNodeByName_WithType(LPCTSTR pStrName, JSON_VALUE_TYPE type, LPCTSTR pStrValue, bool bCaseSensitive);
    bool _setNodeByIndex_WithType(intptr_t nIndex, JSON_VALUE_TYPE type, LPCTSTR pStrValue);
	static void _freeJSON_VALUE(JSON_VALUE& val);
	static bool isIntegerBase10String(LPCTSTR pStr);
	static bool parseFloat(LPCTSTR pStr, double* pfOutVal = nullptr);
	static JSON_NODE_TYPE _determineNodeType(JSON_VALUE* pVal);

#ifdef __APPLE__
    //macOS specific
    static int _compareStringsEqualNoCase_macOS(const char* pStr1,
                                                intptr_t ncbStrLen1,
                                                const char* pStr2,
                                                intptr_t ncbStrLen2);

#endif
};




enum JSON_SPACES
{
	JSP_USE_SPACES,							//Use spaces as tabs (the value for each tab is defined in '')
	JSP_USE_TABS,							//Use tabs
};

enum JSON_ESCAPE_TYPE
{
	JESCT_NO_UNICODE_ESCAPING,				//Do not escape Unicode characters
	JESCT_ESCAPE_CHARS_AFTER_0x80,			//Escape characters with codes after 0x80 (or 128)
	JESCT_ESCAPE_CHARS_AFTER_0x100,			//Escape characters with codes after 0x100 (or 256)
};

struct JSON_FORMATTING
{
    bool bHumanReadable;					//true to format JSON in human readable form
	JSON_SPACES spacesType;					//[Used only if 'bHumanReadable' == true] Type of spaces to use
	int nSpacesPerTab;						//[Used only if 'bHumanReadable' == true and 'spacesType' == JSP_USE_SPACES] Number of spaces per tab (3 by default) -- can be [1 to 64]
    std_wstring strNewLine;				    //[Used only if 'bHumanReadable' == true] New line to use ("\n" by default)
	JSON_ESCAPE_TYPE escapeType;			//Type of escaping to use

	JSON_FORMATTING()
	{
		bHumanReadable = true;
		spacesType = JSP_USE_TABS;
		nSpacesPerTab = 3;
		strNewLine = L("\n");
		escapeType = JESCT_NO_UNICODE_ESCAPING;
	}
};




struct JSON_DATA
{
	JSON_VALUE val;				//Collected JSON data

	JSON_DATA()
	{
	}
	~JSON_DATA()
	{
		//Destructor
		emptyData();
	}

	void emptyData()
	{
		//Frees all data
		_freeJSON_VALUE(val);
	}

    bool getRootNode(JSON_NODE* pOutJNode)
	{
		//'pOutJNode' = if not nullptr, set it to be root node
		//RETURN:
		//		= true if success
		if(pOutJNode)
		{
			if(!val.isEmptyValue())
			{
				pOutJNode->typeNode = JNT_ROOT;
				pOutJNode->strName.clear();
				pOutJNode->pVal = &val;
				pOutJNode->pJSONData = this;

				return true;
			}
		}

		return false;
	}

    bool toString(JSON_FORMATTING* pJFormat = nullptr, std_wstring* pOutStr = nullptr)
	{
		//Convert this struct data to JSON string
		//'pJFormat' = if not nullptr, formatting to use for JSON, or nullptr to use defaults
		//'pOutStr' = if not nullptr, receives formatted JSON (if nullptr, can be used to check correctness of data in 'pJE')
		//RETURN:
		//		= true if success
		//		= false if error (check CJSON::GetLastError() for info)
		return json_toString(this, pJFormat, pOutStr);
	}

private:
	void _freeJSON_VALUE(JSON_VALUE& val);
	static bool json_toString(JSON_DATA* pJE, JSON_FORMATTING* pJFormat, std_wstring* pOutStr);

private:
    //Copy constructor and assignments are NOT available!
    JSON_DATA(const JSON_DATA& s) = delete;
    JSON_DATA& operator = (const JSON_DATA& s) = delete;
};



enum JSON_ENCODING
{
    JENC_ANSI,              //8-byte ANSI encoding (may lead to loss of characters!)
    JENC_UTF_8,             //UTF-8 (default on macOS)
    JENC_UNICODE_16,        //Unicode-16 little-endian (default on Windows)
    JENC_UNICODE_16BE,      //Unicode-16 big-endian

};






class CJSON
{
public:
    static int parseJSON(LPCTSTR pStr, JSON_DATA& outJEs, JSON_ERROR* pJError = nullptr);
    static bool toString(JSON_DATA* pJE, JSON_FORMATTING* pJFormat = nullptr, std_wstring* pOutStr = nullptr);
    static bool parseFloat(LPCTSTR pStr, double* pfOutVal = nullptr);
    static bool isFloatingPointNumberString(LPCTSTR pStr);
    static bool isIntegerBase10String(LPCTSTR pStr);
    static bool getStringForUTF8(LPCTSTR pStr, std::string& strOut);
    static bool getStringForEncoding(LPCTSTR pStr, JSON_ENCODING enc, std::string& strOut
#ifdef _WIN32
                                     , bool* pbOutDataLoss = nullptr
#endif
                                     );
    static bool getUnicodeStringFromEncoding(const char* pAStr, intptr_t ncbLen, JSON_ENCODING enc, std_wstring* pOutUnicodeStr);
    static bool convertStringToUnicode(const char* pAStr, intptr_t ncbLen, JSON_ENCODING enc, std_wstring* pOutUnicodeStr);
    static bool readFileContents(LPCTSTR pStrFilePath, BYTE** ppOutData = nullptr, UINT* pncbOutDataSz = nullptr, UINT ncbSzMaxFileSz = 0);
    static bool readFileContentsAsString(LPCTSTR pStrFilePath, std_wstring* pOutStr = nullptr, UINT ncbSzMaxFileSz = 0);
    static bool writeFileContents(LPCTSTR pStrFilePath, const BYTE* pData, size_t ncbDataSz, const BYTE* pBOMData = nullptr, size_t ncbBOMSz = 0);
    static bool writeFileContentsAsString(LPCTSTR pStrFilePath,
                                          std_wstring* pStr,
                                          JSON_ENCODING enc = JENC_UTF_8
#ifdef _WIN32
                                          ,
                                          bool bAllowAnyDataLoss = false,
                                          bool* pbOutDataLoss = nullptr
#endif
                                          );
    static std_wstring& appendFormat(std_wstring& str, LPCTSTR pszFormat, ...);
    static WCHAR* remove_nulls_from_str(WCHAR* p_str, size_t& szch);
    static std_wstring& lTrim(std_wstring &s);
    static std_wstring& rTrim(std_wstring &s);
    static std_wstring& Trim(std_wstring &s);
    
    static int GetLastError()
    {
#ifdef _WIN32
        //Windows specific
        return ::GetLastError();
#elif __APPLE__
        //macOS specific
        return g_tls_LastErr;
#endif
    }
    
    static void SetLastError(int nError)
    {
#ifdef _WIN32
        //Windows specific
        ::SetLastError(nError);
#elif __APPLE__
        //macOS specific
        g_tls_LastErr = nError;
#endif
    }
    
    
    
private:
    friend struct JSON_DATA;
    friend struct JSON_NODE;
    CJSON(void){};
    ~CJSON(void){};
    
#ifdef __APPLE__
    static thread_local int g_tls_LastErr;
#endif
    
    
#ifdef _WIN32
    //Windows specific

    static bool _isWhiteSpace(WCHAR z)
    {
        //White spaces allowed in JSON
        //RETURN: true if 'z' is a white-space
        return z == ' ' || z == '\t' || z == '\n' || z == '\r';
    }

    static bool _isPlainValueChar(WCHAR z)
    {
        return z == '_' ||
            z == '-' ||
            z == '+' ||
            z == '.' ||
            ::IsCharAlphaNumeric(z);
    }

#elif __APPLE__
//macOS specific

    static bool _isWhiteSpace(UINT z)
    {
        //White spaces allowed in JSON
        //RETURN: true if 'z' is a white-space
        return z == ' ' || z == '\t' || z == '\n' || z == '\r';
    }

	static bool _isPlainValueChar(UINT z)
	{
        if(z == '_' ||
           z == '-' ||
           z == '+' ||
           z == '.')
        {
            return true;
        }

        return std::isalnum(z) != 0;
	}
#endif

	static std_wstring easyFormat(LPCTSTR pszFormat, ...)
	{
		//Format the string
		//INFO: Uses the locale currently set
		int nOSError = CJSON::GetLastError();
		va_list args;
		va_start(args, pszFormat);

        std_wstring str;

#ifdef _WIN32
        //Windows specific
		intptr_t nSz = _vscwprintf(pszFormat, args);
        
#elif __APPLE__
        //macOS specific
        intptr_t nSz = vprintf(pszFormat, args);
#endif
        
		if(nSz >= 0)
        {
			WCHAR* p_buff = new (std::nothrow) WCHAR[nSz + 1];
			if(p_buff)
            {
				p_buff[0] = 0;
                
#ifdef _WIN32
                //Windows specific
				vswprintf_s(p_buff, nSz + 1, pszFormat, args);
#elif __APPLE__
                //macOS specific
                vsnprintf(p_buff, nSz + 1, pszFormat, args);
#endif

				str.assign(p_buff, nSz);

				delete[] p_buff;
			}
		}

		va_end(args);

        CJSON::SetLastError(nOSError);
		return str;
	}
	static WCHAR _skipWhiteSpaces(const WCHAR* pData, intptr_t& i, intptr_t nLen);
	static int _parseForArray(JSON_ARRAY& ja, const WCHAR* pData, intptr_t& i, intptr_t nLen, JSON_ERROR* pJError);
	static int _parseForObject(JSON_OBJECT& jo, const WCHAR* pData, intptr_t& i, intptr_t nLen, JSON_ERROR* pJError);
	static int _parseDoubleQuotedString(std_wstring& str, const WCHAR* pData, intptr_t& i, intptr_t nLen, JSON_ERROR* pJError);
	static int _parseForValue(JSON_VALUE& jv, const WCHAR* pData, intptr_t& i, intptr_t nLen, JSON_ERROR* pJError);
	static size_t _toString_Value(JSON_VALUE& val, JSON_FORMATTING* pJFormat, std_wstring* pOutStr, intptr_t nIndent);
	static size_t _escapeDoubleQuotedVal(std_wstring& s, JSON_FORMATTING* pJFormat, std_wstring* pOutStr = nullptr);
	static void _freeJSON_ARRAY(JSON_ARRAY* pJA);
	static void _freeJSON_OBJECT(JSON_OBJECT* pJO);
	static void _freeJSON_VALUE(JSON_VALUE& val);
	static void _describeError(JSON_ERROR* pJError, intptr_t i, LPCTSTR pErrDesc = nullptr);
	static JSON_NODE_TYPE _determineNodeTypeSafe(JSON_VALUE* pVal);
	static JSON_NODE_TYPE _determineNodeType(JSON_VALUE* pVal);
	static bool _deepCopyJSON_VALUE(JSON_VALUE* pDestV, JSON_VALUE* pSrcV);
	static bool __copySingleVal(JSON_VALUE* pDestV, JSON_VALUE* pSrcV);
};

};
