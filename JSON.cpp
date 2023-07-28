//Class that implements JSON parser
//
// (C) 2015, www.dennisbabkin.com
//


//JSON specs:
//		http://tools.ietf.org/html/rfc7158
//


#include "JSON.h"


namespace json
{


#ifdef __APPLE__
int thread_local CJSON::g_tls_LastErr;
#endif



int CJSON::parseJSON(LPCTSTR pStr, JSON_DATA& outJEs, JSON_ERROR* pJError)
{
    //Parse 'pStr' as JSON
    //'outJEs' = receives parsed JSON data -- must be newly created
    //'pJError' = if not nullptr, will be filled with parsing error details
    //RETURN:
    //		= 1 if got it OK
    //		= 0 if JSON format error
    //		= -1 if other non-JSON related error (such as out of memory, etc.)
    //           INFO: Check CJSON::GetLastError() for more info.
    int nRes = -1;

    if(pStr)
    {
        //Clear the data variable
        outJEs.emptyData();

        //Begin
        intptr_t i = 0;
        intptr_t nLen = STRLEN(pStr);

        //Reset last error before we begin
        CJSON::SetLastError(0);

        //Go to next non-white-space
        WCHAR c = _skipWhiteSpaces(pStr, i, nLen);
        if(c)
        {
            //Begin from the root object
            nRes = _parseForValue(outJEs.val, pStr, i, nLen, pJError);
            if(nRes == 1)
            {
                //Skip to the end
                if(_skipWhiteSpaces(pStr, i, nLen) != 0)
                {
                    //Something else follows { ... } main root object
                    ASSERT(nullptr);
                    _describeError(pJError, i, L("Unexpected data after the root node"));
                    nRes = 0;
                }
            }
        }
        else
        {
            //Error
            ASSERT(nullptr);
            _describeError(pJError, i, L("Unexpected EOF"));
            nRes = 0;
        }
    }
    else
    {
        _describeError(pJError, -1, L("Bad input parameter(s)"));
        CJSON::SetLastError(ERROR_INVALID_PARAMETER);
    }

    return nRes;
}




WCHAR CJSON::_skipWhiteSpaces(const WCHAR* pData, intptr_t& i, intptr_t nLen)
{
    //Update 'i' to point to the next non-white space WCHAR
    //'pData' = beginning of JSON string to parse
    //'i' = index of the WCHAR to begin skipping white spaces from
    //		INFO: It will be updated upon return to point to the first non-space char on or after original 'i'
    //'nLen' = length of 'pData' in TCHARs
    //RETURN:
    //		= Non-0 for the next non-white-space WCHAR on or after 'i' (and 'i' will point to this non-white-space char in 'pData')
    //		= 0 if end-of-data is reached (and 'i' is out of scope)
    for(; i < nLen; i++)
    {
        WCHAR z = pData[i];
        ASSERT(z);
        if(_isWhiteSpace(z))
            continue;

        return z;
    }

    //End of file
    return 0;
}



int CJSON::_parseDoubleQuotedString(std_wstring& str, const WCHAR* pData, intptr_t& i, intptr_t nLen, JSON_ERROR* pJError)
{
    //Parse double quoted string into 'str'
    //'pData' = beginning of JSON string to parse
    //'i' = index of the '"' WCHAR to begin parsing the "string" from
    //		INFO: It will be updated upon return to point to the char one after the last one in the "string"
    //'nLen' = length of 'pData' in TCHARs
    //RETURN:
    //		= 1 if got it OK, 'i' points to the next WCHAR after the "string"
    //		= 0 if format error, 'i' may be out of range
    //		= -1 if other non-JSON related error (such as out of memory, check CJSON::GetLastError() for info)

    //Clear the string
    str.clear();

    WCHAR buffHex[5];
    buffHex[SIZEOF(buffHex) - 1] = 0;

    intptr_t i_delta = 1;
    
    //Fill out string
    for(i++;; i += i_delta)
    {
        if(i >= nLen)
        {
            //Reached EOF
            ASSERT(nullptr);
            _describeError(pJError, i, L("Unexpected EOF"));
            return 0;
        }

#ifdef _WIN32
//Windows specific
        WCHAR z = pData[i];
        
#elif __APPLE__
//macOS specific
        
        UINT z;
        i_delta = JSON_NODE::getUtf8Char(pData, i, nLen, &z);
        if(i_delta <= 0)
        {
            //Error
            ASSERT(nullptr);
            _describeError(pJError, i, L("Bad UTF-8 sequence"));
            return 0;
        }
#endif
        
        if(z == '"')
        {
            //End of string reached
            i += i_delta;

            return 1;
        }
        else if(z == '\n' || z == '\r')
        {
            //Error in formatting
            ASSERT(nullptr);
            _describeError(pJError, i, L("Newline in quote"));
            return 0;
        }
        else if(z == '\\')
        {
            //Escaping, get next char
            i += i_delta;
            
            if(i >= nLen)
            {
                //Error
                ASSERT(nullptr);
                _describeError(pJError, i, L("Unexpected EOF"));
                return 0;
            }

            //Get next char, look for special cases
            //%x22 /          ; "    quotation mark  U+0022
            //%x5C /          ; \    reverse solidus U+005C
            //%x2F /          ; /    solidus         U+002F
            //%x62 /          ; b    backspace       U+0008
            //%x66 /          ; f    form feed       U+000C
            //%x6E /          ; n    line feed       U+000A
            //%x72 /          ; r    carriage return U+000D
            //%x74 /          ; t    tab             U+0009
            //%x75 4HEXDIG )  ; uXXXX                U+XXXX
            ASSERT('\b' == 0x0008);
            ASSERT('\f' == 0x000C);
            ASSERT('\n' == 0x000A);
            ASSERT('\r' == 0x000D);
            ASSERT('\t' == 0x0009);

            z = pData[i];
            if(z == '"' ||
                z == '\\' ||
                z == '/')
            {
                //Keep it as is
            }
            else if(z == 'b')
            {
                z = '\b';
            }
            else if(z == 'f')
            {
                z = '\f';
            }
            else if(z == 'n')
            {
                z = '\n';
            }
            else if(z == 'r')
            {
                z = '\r';
            }
            else if(z == 't')
            {
                z = '\t';
            }
            else if(z == 'u')
            {
                //\u005C
                if(i + 4 >= nLen)
                {
                    //Error
                    ASSERT(nullptr);
                    _describeError(pJError, i, L("Unexpected EOF"));
                    return 0;
                }

                buffHex[0] = pData[++i];
                buffHex[1] = pData[++i];
                buffHex[2] = pData[++i];
                buffHex[3] = pData[++i];

                UINT uZ = 0;

#ifdef _WIN32
//Windows specific
                if(_stscanf_s(buffHex, L"%x", &uZ) != 1 ||
                    uZ > 0xFFFF)
                {
                    //Error
                    ASSERT(nullptr);
                    _describeError(pJError, i, L("Failed to unescape"));
                    return 0;
                }

                z = (WCHAR)uZ;

#elif __APPLE__
//macOS specific
                if(sscanf(buffHex, "%x", &uZ) != 1 ||
                   uZ > UTF8_MAX_VAL)
                {
                    //Error
                    ASSERT(nullptr);
                    _describeError(pJError, i, L("Failed to unescape"));
                    return 0;
                }
                
                z = uZ;
#endif
            }
            else
            {
                //Technically an error = but don't quit
                ASSERT(nullptr);		//Bad escaping
            }
        }

        //Add it to name
#ifdef _WIN32
//Windows specific
        str += z;
        
#elif __APPLE__
//macOS specific
        if(!JSON_NODE::appendUtf8Char(str, z))
        {
            //Failed
            ASSERT(nullptr);
            _describeError(pJError, i, L("Failed to add UTF-8 character"));
            return 0;
        }
#endif
    }

    ASSERT(nullptr);
    _describeError(pJError, i, L("Bad execution branch"));
    CJSON::SetLastError(ERROR_HANDLE_EOF);
    return -1;
}


int CJSON::_parseForArray(JSON_ARRAY& ja, const WCHAR* pData, intptr_t& i, intptr_t nLen, JSON_ERROR* pJError)
{
    //Parse for array
    //'ja' = array to parse into -- must be freshly created
    //'pData' = beginning of JSON string to parse
    //'i' = index of the WCHAR to begin parsing from (may be space) -- must be the char right after the opening '['
    //		INFO: It will be updated upon return to point to the char one after the last one in the array, i.e. ']'
    //'nLen' = length of 'pData' in TCHARs
    //RETURN:
    //		= 1 if got it OK, 'i' points to the next WCHAR after the value
    //		= 0 if format error, 'i' may be out of range
    //		= -1 if other non-JSON related error (such as out of memory, check CJSON::GetLastError() for info)
    int nR;

    bool bGotPreviousComma = true;

    for(intptr_t cnt = 0;; cnt++)
    {
        //Go to next non-white-space
        WCHAR c = _skipWhiteSpaces(pData, i, nLen);
        if(!c)
        {
            //Reached EOF too early
            ASSERT(nullptr);
            _describeError(pJError, i, L("Unexpected EOF"));
            return 0;
        }

        if(c == ']')
        {
            //End of array
            i++;

            return 1;
        }
        else if(c == ',')
        {
            //Comma separator between elements (only if not the first element)
            if(cnt == 0 ||
                bGotPreviousComma)
            {
                //Error
                ASSERT(nullptr);
                _describeError(pJError, i, L("Unexpected comma"));
                return 0;
            }

            //Signal that we got it
            bGotPreviousComma = true;

            //Otherwise skip it
            i++;
            continue;
        }

        //Make sure that we've got a comma before
        if(!bGotPreviousComma)
        {
            //Error - missing comma
            ASSERT(nullptr);
            _describeError(pJError, i, L("Expected a comma"));
            return 0;
        }

        JSON_ARRAY_ELEMENT jae;

        //Parse value
        nR = _parseForValue(jae.val, pData, i, nLen, pJError);
        if(nR != 1)
        {
            //Error
            ASSERT(nullptr);
            _describeError(pJError, i, L("Value parsing failed"));
            return nR;
        }

        //Add it
        ja.arrArrElmts.push_back(jae);

        //Reset comma flag
        bGotPreviousComma = false;
    }

//	_describeError(pJError, i, L("Bad execution branch"));
//    CJSON::SetLastError(ERROR_INVALID_DATA);
//	return -1;
}



int CJSON::_parseForObject(JSON_OBJECT& jo, const WCHAR* pData, intptr_t& i, intptr_t nLen, JSON_ERROR* pJError)
{
    //Parse for object
    //'jo' = object to parse into -- must be freshly created
    //'pData' = beginning of JSON string to parse
    //'i' = index of the WCHAR to begin parsing from (may be space) -- must be the char right after the opening '{'
    //		INFO: It will be updated upon return to point to the char one after the last one in the object, i.e. '}'
    //'nLen' = length of 'pData' in TCHARs
    //RETURN:
    //		= 1 if got it OK, 'i' points to the next WCHAR after the object
    //		= 0 if format error, 'i' may be out of range
    //		= -1 if other non-JSON related error (such as out of memory, check CJSON::GetLastError() for info)
    int nR;

    bool bGotPreviousComma = true;

    for(intptr_t cnt = 0;; cnt++)
    {
        //Go to next non-white-space
        WCHAR c = _skipWhiteSpaces(pData, i, nLen);
        if(!c)
        {
            //Reached EOF too early
            ASSERT(nullptr);
            _describeError(pJError, i, L("Unexpected EOF"));
            return 0;
        }

        if(c == '}')
        {
            //End of object
            i++;

            return 1;
        }
        else if(c == ',')
        {
            //Comma separator between elements (only if not the first element)
            if(cnt == 0 ||
                bGotPreviousComma)
            {
                //Error
                ASSERT(nullptr);
                _describeError(pJError, i, L("Unexpected comma"));
                return 0;
            }

            //Signal that we got it
            bGotPreviousComma = true;

            //Otherwise skip it
            i++;
            continue;
        }
        else if(c == '"')
        {
            //Name

            //Make sure that we've got a comma before
            if(!bGotPreviousComma)
            {
                //Error - missing comma
                ASSERT(nullptr);
                _describeError(pJError, i, L("Expected a comma"));
                return 0;
            }

            JSON_OBJECT_ELEMENT joe;

            //Parse name
            nR = _parseDoubleQuotedString(joe.strName, pData, i, nLen, pJError);
            if(nR != 1)
            {
                //Error
                ASSERT(nullptr);
                _describeError(pJError, i, L("Quote parsing failed"));
                return nR;
            }
            
            //Go to next non-white-space
            c = _skipWhiteSpaces(pData, i, nLen);
            if(!c)
            {
                //Reached EOF too early
                ASSERT(nullptr);
                _describeError(pJError, i, L("Unexpected EOF"));
                return 0;
            }

            if(c != ':')
            {
                //Wrong format
                ASSERT(nullptr);
                _describeError(pJError, i, L("Expected a colon"));
                return 0;
            }
            
            i++;

            //Go to next non-white-space
            c = _skipWhiteSpaces(pData, i, nLen);
            if(!c)
            {
                //Reached EOF too early
                ASSERT(nullptr);
                _describeError(pJError, i, L("Unexpected EOF"));
                return 0;
            }

            //Parse value
            nR = _parseForValue(joe.val, pData, i, nLen, pJError);
            if(nR != 1)
            {
                //Error
                ASSERT(nullptr);
                _describeError(pJError, i, L("Value parsing failed"));
                return nR;
            }

            //Add it to the object
            jo.arrObjElmts.push_back(joe);

            //Reset comma flag
            bGotPreviousComma = false;
        }
        else
        {
            //Error
            ASSERT(nullptr);
            _describeError(pJError, i, L("Unexpected formatting character"));
            return 0;
        }
    }

//    CJSON::SetLastError(ERROR_INVALID_DATA);
//	return -1;
}


int CJSON::_parseForValue(JSON_VALUE& jv, const WCHAR* pData, intptr_t& i, intptr_t nLen, JSON_ERROR* pJError)
{
    //Parse for value in "name" : "value" JSON pair
    //'jv' = will be filled out with the value
    //'pData' = beginning of JSON string to parse
    //'i' = index of the WCHAR right after ':' in the example above -- it will be updated upon return to point to the char one after last in the "value"
    //'nLen' = length of 'pData' in TCHARs
    //RETURN:
    //		= 1 if got it OK, 'i' points to the next WCHAR after the "value"
    //		= 0 if format error, 'i' may be out of range
    //		= -1 if other non-JSON related error (such as out of memory, check CJSON::GetLastError() for info)
    int nR;

    //Go to next non-white-space
    if(!_skipWhiteSpaces(pData, i, nLen))
    {
        //Reached EOF too early
        ASSERT(nullptr);
        _describeError(pJError, i, L("Unexpected EOF"));
        return 0;
    }

    intptr_t i_delta;
    
#ifdef _WIN32
//Windows specific
    
    WCHAR c = pData[i];
    i_delta = 1;
    
#elif __APPLE__
//macOS specific
    
    UINT c;
    i_delta = JSON_NODE::getUtf8Char(pData, i, nLen, &c);
    if(i_delta <= 0)
    {
        //Error
        ASSERT(nullptr);
        _describeError(pJError, i, L("Bad UTF-8 sequence"));
        return 0;
    }

#endif

    //See what type of value is it
    if(c == '"')
    {
        //Begin quoted value
        jv.valType = JVT_DOUBLE_QUOTED;

        //Parse it
        nR = _parseDoubleQuotedString(jv.strValue, pData, i, nLen, pJError);
        if(nR != 1)
        {
            //Failed
            ASSERT(nullptr);
            _describeError(pJError, i, L("Quote parsing failed"));
            return nR;
        }

    }
    else if(_isPlainValueChar(c))
    {
        //Begin plain value
        jv.valType = JVT_PLAIN;
        jv.strValue = c;

        //Look for the end
        for(i += i_delta; ; i += i_delta)
        {
            if(i >= nLen)
            {
                //Reached EOF, it's OK
                break;
            }

#ifdef _WIN32
//Windows specific
    
            WCHAR z = pData[i];
            
#elif __APPLE__
//macOS specific
    
            UINT z;
            i_delta = JSON_NODE::getUtf8Char(pData, i, nLen, &z);
            if(i_delta <= 0)
            {
                //Error
                ASSERT(nullptr);
                _describeError(pJError, i, L("Bad UTF-8 sequence"));
                return 0;
            }
#endif

            if(_isWhiteSpace(z) ||
                z == ',' ||
                z == '}' ||
                z == ']')
            {
                //End of value
                break;
            }

            //Chars must be formatted correctly
            ASSERT(_isPlainValueChar(z));

#ifdef _WIN32
//Windows specific
    
            jv.strValue += z;
            
#elif __APPLE__
//macOS specific
            
            if(!JSON_NODE::appendUtf8Char(jv.strValue, z))
            {
                //Failed
                ASSERT(nullptr);
                _describeError(pJError, i, L("Failed to add UTF-8 character"));
                return 0;
            }
#endif
        }
    }
    else if(c == '[')
    {
        //Begain array
        jv.strValue.clear();

        i += i_delta;

        //Create new array
        JSON_ARRAY* pJA = new (std::nothrow) JSON_ARRAY;
        if(!pJA)
        {
            //Out of memory
            ASSERT(nullptr);
            _describeError(pJError, i, L("Out of memory"));
            CJSON::SetLastError(ERROR_OUTOFMEMORY);
            return -1;
        }

        //Parse it
        nR = _parseForArray(*pJA, pData, i, nLen, pJError);
        if(nR != 1)
        {
            //Error
            int nErr = CJSON::GetLastError();
            ASSERT(nullptr);
            _describeError(pJError, i, L("Array parsing failed"));

            //Delete it
            _freeJSON_ARRAY(pJA);

            CJSON::SetLastError(nErr);
            return nR;
        }

        //Mark it
        jv.valType = JVT_ARRAY;

        //Add it then
        jv.pValue = pJA;
    }
    else if(c == '{')
    {
        //Begin object
        jv.strValue.clear();

        i += i_delta;

        //Create new object
        JSON_OBJECT* pJO = new (std::nothrow) JSON_OBJECT;
        if(!pJO)
        {
            //Out of memory
            ASSERT(nullptr);
            _describeError(pJError, i, L("Out of memory"));
            CJSON::SetLastError(ERROR_OUTOFMEMORY);
            return -1;
        }

        //Parse it
        nR = _parseForObject(*pJO, pData, i, nLen, pJError);
        if(nR != 1)
        {
            //Error
            int nErr = CJSON::GetLastError();
            ASSERT(nullptr);
            _describeError(pJError, i, L("Object parsing failed"));

            //Delete it then
            _freeJSON_OBJECT(pJO);

            CJSON::SetLastError(nErr);
            return nR;
        }

        //Mark it
        jv.valType = JVT_OBJECT;

        //Add it then
        jv.pValue = pJO;
    }
    else
    {
        //Error in format
        ASSERT(nullptr);
        _describeError(pJError, i, L("Unexpected formatting character"));
        return 0;
    }

    return 1;
}


bool JSON_DATA::json_toString(JSON_DATA* pJE, JSON_FORMATTING* pJFormat, std_wstring* pOutStr)
{
    //Redirect
    return CJSON::toString(pJE, pJFormat, pOutStr);
}

bool CJSON::toString(JSON_DATA* pJE, JSON_FORMATTING* pJFormat, std_wstring* pOutStr)
{
    //Convert 'pJE' to JSON string
    //'pJFormat' = if not nullptr, formatting to use for JSON, or nullptr to use defaults
    //'pOutStr' = if not nullptr, receives formatted JSON (if nullptr, can be used to check correctness of data in 'pJE')
    //RETURN:
    //		= true if success
    //		= false if error (check CJSON::GetLastError() for info)
    bool bRes = false;
    int nOSError = NO_ERROR;

    if(pJE)
    {
        //Do we have a formatting struct
        JSON_FORMATTING jFmt;
        if(!pJFormat)
        {
            //Use defaults
            pJFormat = &jFmt;
        }

        //Empty string
        if(pOutStr)
            pOutStr->clear();

        //Measure how much data do we need
        //INFO: We need this to speed up _toString_Value() method!
        //		If 'pOutStr' == nullptr then:
        //			0 if success
        //			-1 if error
        //		If 'pOutStr' != nullptr then:
        //			[0 and up) number of TCHARs required for this value
        //			-1 if error
        size_t n_res_cnt = _toString_Value(pJE->val, pJFormat, nullptr, 1);

        if(n_res_cnt != -1)
        {
            //Only if we have a pointer
            if(pOutStr)
            {
                //And print
                size_t n_res_cnt2 = _toString_Value(pJE->val, pJFormat, pOutStr, 1);
                if(n_res_cnt2 != -1)
                {
                    //Check that we calculate the size correctly
                    //INFO: It is OK if we miscalculated it a little bit...
                    size_t nCntStrChk = pOutStr->size();
                    ASSERT(nCntStrChk == n_res_cnt);

                    //Done
                    bRes = true;
                }
                else
                {
                    //Error
                    nOSError = ERROR_BAD_FORMAT;
                    pOutStr->clear();
                }
            }
            else
            {
                //Then success
                bRes = true;
            }
        }
        else
        {
            //Failed
            nOSError = ERROR_BAD_FORMAT;

            if(pOutStr)
            {
                //If failed, clear the string
                pOutStr->clear();
            }
        }
    }
    else
        nOSError = ERROR_INVALID_PARAMETER;

    CJSON::SetLastError(nOSError);
    return bRes;
}


size_t CJSON::_toString_Value(JSON_VALUE& val, JSON_FORMATTING* pJFormat, std_wstring* pOutStr, intptr_t nIndent)
{
    //RETURN:
    //		If 'pOutStr' == nullptr then:
    //			0 if success
    //			-1 if error
    //		If 'pOutStr' != nullptr then:
    //			[0 and up) number of TCHARs required for this value
    //			-1 if error
    size_t nResCount = 0;
    
    std_wstring strTabIndent, strTabIndent_1;
    bool bHumanReadable = pJFormat->bHumanReadable;
    if(bHumanReadable)
    {
        //Prep tab index
        std_wstring strTab;
        if(pJFormat->spacesType == JSP_USE_SPACES)
        {
            int nNmSps = pJFormat->nSpacesPerTab;
            if(nNmSps < 1)
                nNmSps = 1;
            else if(nNmSps > 64)
                nNmSps = 64;

            for(int s = 0; s < nNmSps; s++)
            {
                strTab += ' ';
            }
        }
        else
            strTab = '\t';

        for(intptr_t t = 0; t < nIndent - 1; t++)
        {
            strTabIndent_1 += strTab;
        }

        strTabIndent = strTabIndent_1 + strTab;
    }

    switch(val.valType)
    {
    case JVT_PLAIN:					// 25, 167.6, 12E40, -12, +12, true, false, null
        {
            if(pOutStr)
                pOutStr->operator +=(val.strValue);
            else
                nResCount += val.strValue.size();
        }
        break;

    case JVT_DOUBLE_QUOTED:			// "string"
        {
            if(pOutStr)
            {
                pOutStr->operator +=('"');
                _escapeDoubleQuotedVal(val.strValue, pJFormat, pOutStr);
                pOutStr->operator +=('"');
            }
            else
            {
                nResCount += 2 + _escapeDoubleQuotedVal(val.strValue, pJFormat);
            }
        }
        break;

    case JVT_ARRAY:					// [ val1, val2 ]
        {
            if(pOutStr)
            {
                pOutStr->operator +=('[');
            }
            else
                nResCount += 1;

            JSON_ARRAY* pJA = (JSON_ARRAY*)val.pValue;
            if(pJA)
            {
                intptr_t nCnt = pJA->arrArrElmts.size();
                for(intptr_t i = 0; i < nCnt; i++)
                {
                    //Print each element
                    size_t n_res_chrsA = _toString_Value(pJA->arrArrElmts[i].val, pJFormat, pOutStr, nIndent);
                    if(n_res_chrsA != -1)
                    {
                        //Add to other value
                        nResCount += n_res_chrsA;
                    }
                    else
                    {
                        //Failed
                        ASSERT(nullptr);
                        nResCount = -1;
                        break;
                    }

                    if(i + 1 < nCnt)
                    {
                        //Add comma
                        if(pOutStr)
                            pOutStr->operator +=(',');
                        else
                            nResCount += 1;

                        if(bHumanReadable)
                        {
                            if(pOutStr)
                                pOutStr->operator +=(' ');
                            else
                                nResCount += 1;
                        }
                    }
                }
            }
            else
            {
                //Error
                ASSERT(nullptr);
                nResCount = -1;
            }

            if(pOutStr)
                pOutStr->operator +=(L']');
            else
                nResCount += 1;
        }
        break;

    case JVT_OBJECT:				// { "name1":"value1", "name2" : "value2" }
        {
            if(pOutStr)
                pOutStr->operator +=('{');
            else
                nResCount += 1;

            JSON_OBJECT* pJO = (JSON_OBJECT*)val.pValue;
            if(pJO)
            {
                intptr_t nCnt = pJO->arrObjElmts.size();
                JSON_OBJECT_ELEMENT* pJOEs = pJO->arrObjElmts.data();

                for(intptr_t i = 0; i < nCnt; i++)
                {
                    //Element name
                    if(bHumanReadable &&
                        i == 0)
                    {
                        if(pOutStr)
                        {
                            pOutStr->operator +=(pJFormat->strNewLine);
                            pOutStr->operator +=(strTabIndent);
                        }
                        else
                            nResCount += pJFormat->strNewLine.size() + strTabIndent.size();
                    }

                    if(pOutStr)
                    {
                        CJSON::appendFormat(*pOutStr, 
                            L("\"%s\":%s"),
                            pJOEs[i].strName.c_str(),
                            bHumanReadable ? L(" ") : L("")
                        );
                    }
                    else
                        nResCount += 1 + pJOEs[i].strName.size() + 1 + 1 + (bHumanReadable ? 1 : 0);

                    //Print value
                    size_t n_res_chrsO = _toString_Value(pJOEs[i].val, pJFormat, pOutStr, nIndent + 1);
                    if(n_res_chrsO != -1)
                    {
                        //Add to other value
                        nResCount += n_res_chrsO;
                    }
                    else
                    {
                        //Failed
                        ASSERT(nullptr);
                        nResCount = -1;
                        break;
                    }

                    if(i + 1 < nCnt)
                    {
                        //Add comma
                        if(pOutStr)
                            pOutStr->operator +=(',');
                        else
                            nResCount += 1;

                        if(bHumanReadable)
                        {
                            if(pOutStr)
                            {
                                pOutStr->operator +=(pJFormat->strNewLine);
                                pOutStr->operator +=(strTabIndent);
                            }
                            else
                                nResCount += pJFormat->strNewLine.size() + strTabIndent.size();
                        }
                    }
                    else if(i + 1 == nCnt &&
                        bHumanReadable)
                    {
                        //Last element
                        if(pOutStr)
                        {
                            pOutStr->operator +=(pJFormat->strNewLine);
                            pOutStr->operator +=(strTabIndent_1);
                        }
                        else
                            nResCount += pJFormat->strNewLine.size() + strTabIndent_1.size();
                    }
                }
            }
            else
            {
                //Error
                ASSERT(nullptr);
                nResCount = -1;
            }

            if(pOutStr)
                pOutStr->operator +=(L'}');
            else
                nResCount += 1;
        }
        break;

    default:
        {
            //Bad type
            ASSERT(nullptr);
            nResCount = -1;
        }
        break;
    }

    return nResCount;
}

std_wstring& CJSON::appendFormat(std_wstring& str, LPCTSTR pszFormat, ...)
{
	//Format string the same way as wsprintf() does
	int nOSError = CJSON::GetLastError();

	va_list args;
	va_start(args, pszFormat);

    va_list args2;
    va_copy(args2, args);

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
			vswprintf_s(p_buff, nSz + 1, pszFormat, args2);
#elif __APPLE__
            //macOS specific
            vsnprintf(p_buff, nSz + 1, pszFormat, args2);
#endif
			remove_nulls_from_str(p_buff, (size_t&)nSz);
			str.append(p_buff, nSz);

			delete[] p_buff;
		}
	}

    va_end(args);
    va_end(args2);

    CJSON::SetLastError(nOSError);

	return str;
}

WCHAR* CJSON::remove_nulls_from_str(WCHAR* p_str, size_t& szch)
{
    //Remove all internal nulls that may be in 'p_str'
    //'szch' = original size of 'p_str' in WCHARs, without last null. It may be adjusted after this function returns
    //RETURN:
    //		= Pointer to 'p_str'

    if(p_str &&
        szch)
    {
        size_t szch0 = szch;

        WCHAR* pE = p_str + szch;
        for(WCHAR* pS = pE - 1; pS >= p_str; pS--)
        {
            if(!*pS)
            {
                if(pS + 1 < pE)
                {
                    //Remove it
                    memcpy(pS, pS + 1, (pE - pS - 1) * sizeof(WCHAR));
                }

                szch--;
            }
        }

        //Set last null
        if(szch < szch0)
        {
            //Last null
            p_str[szch] = 0;
        }
    }

    return p_str;
}


std_wstring& CJSON::lTrim(std_wstring &s)
{
    //Trim all white-spaces from the left side of 's'
    //RETURN: = Same trimmed string
    
#ifdef _WIN32
    //Windows specific
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    
#elif __APPLE__
    //macOS specific
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) {return !std::isspace(c);}));

#endif
    
    return s;
}

std_wstring& CJSON::rTrim(std_wstring &s)
{
    //Trim all white-spaces from the right side of 's'
    //RETURN: = Same trimmed string

#ifdef _WIN32
    //Windows specific
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());

#elif __APPLE__
    //macOS specific
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int c) {return !std::isspace(c);}).base(), s.end());
    
#endif

    return s;
}

std_wstring& CJSON::Trim(std_wstring &s)
{
    //Trim all white-spaces from 's'
    //RETURN: = Same trimmed string
    return lTrim(rTrim(s));
}



size_t CJSON::_escapeDoubleQuotedVal(std_wstring& s, JSON_FORMATTING* pJFormat, std_wstring* pOutStr)
{
    //'pOutStr' = if specified, will receive escaped string
    //			= if nullptr, will return length of required string
    //RETURN:
    //		= Size of escaped quote in TCHARs if 'pOutStr' == nullptr
    //		= 0 if 'pOutStr' != nullptr, or if error
    ASSERT(pJFormat);
    size_t nResCnt = 0;
     
    JSON_ESCAPE_TYPE escTp = pJFormat ? pJFormat->escapeType : JESCT_NO_UNICODE_ESCAPING;

    ASSERT('\b' == 0x0008);
    ASSERT('\f' == 0x000C);
    ASSERT('\n' == 0x000A);
    ASSERT('\r' == 0x000D);
    ASSERT('\t' == 0x0009);

    intptr_t i_delta = 1;
    
    intptr_t nLn = s.size();
    LPCTSTR pStr = s.c_str();

    for(intptr_t i = 0; i < nLn; i += i_delta)
    {
#ifdef _WIN32
//Windows specific
        WCHAR z = pStr[i];
        
#elif __APPLE__
//macOS specific
        UINT z;
        i_delta = JSON_NODE::getUtf8Char(pStr, i, nLn, &z);
        if(i_delta <= 0)
        {
            //Error
            ASSERT(nullptr);
            if(pOutStr)
                pOutStr->clear();
            
            return 0;
        }
#endif
        
        //%x22 /          ; "    quotation mark  U+0022
        //%x5C /          ; \    reverse solidus U+005C
        //%x2F /          ; /    solidus         U+002F
        //%x62 /          ; b    backspace       U+0008
        //%x66 /          ; f    form feed       U+000C
        //%x6E /          ; n    line feed       U+000A
        //%x72 /          ; r    carriage return U+000D
        //%x74 /          ; t    tab             U+0009
        //%x75 4HEXDIG )  ; uXXXX                U+XXXX
        if(z == '"' ||
            z == '\\' ||
            z == '/')
        {
            if(pOutStr)
            {
                pOutStr->operator +=('\\');
                pOutStr->operator +=((WCHAR)z);
            }
            else
                nResCnt += TSIZEOF(L("\\z"));
        }
        else if(z == '\b')
        {
            if(pOutStr)
                pOutStr->operator +=(L("\\b"));
            else
                nResCnt += TSIZEOF(L("\\b"));
        }
        else if(z == '\f')
        {
            if(pOutStr)
                pOutStr->operator +=(L("\\f"));
            else
                nResCnt += TSIZEOF(L("\\f"));
        }
        else if(z == '\n')
        {
            if(pOutStr)
                pOutStr->operator +=(L("\\n"));
            else
                nResCnt += TSIZEOF(L("\\n"));
        }
        else if(z == '\r')
        {
            if(pOutStr)
                pOutStr->operator +=(L("\\r"));
            else
                nResCnt += TSIZEOF(L("\\r"));
        }
        else if(z == '\t')
        {
            if(pOutStr)
                pOutStr->operator +=(L("\\t"));
            else
                nResCnt += TSIZEOF(L("\\t"));
        }
        else if(z == 0)
        {
            //nullptr if it's there for some reason
            if(pOutStr)
                pOutStr->operator +=(L("\\0"));
            else
                nResCnt += TSIZEOF(L("\\0"));
        }
        else
        {
            //Do we need to do the escaping?
            bool bEscIt;

#ifdef _WIN32
            //Windows specific
            bEscIt = (z >= 0x80 && escTp == JESCT_ESCAPE_CHARS_AFTER_0x80) ||
                     (z >= 0x100 && escTp == JESCT_ESCAPE_CHARS_AFTER_0x100);
            
#elif __APPLE__
            //macOS specific
            //INFO: We can't escape UTF-8 characters larger than 2 bytes!
            bEscIt = (z >= 0x80 && z <= 0xffff && escTp == JESCT_ESCAPE_CHARS_AFTER_0x80) ||
                     (z >= 0x100 && z <= 0xffff && escTp == JESCT_ESCAPE_CHARS_AFTER_0x100);
#endif
            
            if(bEscIt)
            {
                //Escape Unicode
                ASSERT(z >= 0 && z <= 0xffff);

                if(pOutStr)
                {
                    CJSON::appendFormat(*pOutStr, L("\\u%04x"), z);
                }
                else
                    nResCnt += TSIZEOF(L("\\u0000"));
            }
            else
            {
                //Just add it as-is
#ifdef _WIN32
                //Windows specific
                if(pOutStr)
                    pOutStr->operator +=(z);
                else
                    nResCnt += TSIZEOF(L("z"));
                
#elif __APPLE__
                //macOS specific
                if(pOutStr)
                {
                    if(!JSON_NODE::appendUtf8Char(*pOutStr, z))
                    {
                        //Error
                        ASSERT(nullptr);
                        if(pOutStr)
                            pOutStr->clear();
                        
                        return 0;
                    }
                }
                else
                    nResCnt += i_delta;

#endif
            }
        }
    }

    return nResCnt;
}

void CJSON::_freeJSON_ARRAY(JSON_ARRAY* pJA)
{
    //INFO: When this method returns 'pJA' will be no longer valid!
    if(pJA)
    {
        for(intptr_t i = 0; i < (intptr_t)pJA->arrArrElmts.size(); i++)
        {
            //Free its children first
            _freeJSON_VALUE(pJA->arrArrElmts[i].val);
        }

        //Then free the array
        delete pJA;
    }
}

void CJSON::_freeJSON_OBJECT(JSON_OBJECT* pJO)
{
    //INFO: When this method returns 'pJO' will be no longer valid!
    if(pJO)
    {
        for(intptr_t i = 0; i < (intptr_t)pJO->arrObjElmts.size(); i++)
        {
            //Free its children first
            _freeJSON_VALUE(pJO->arrObjElmts[i].val);
        }

        //Then free the object
        delete pJO;
    }
}

void JSON_DATA::_freeJSON_VALUE(JSON_VALUE& val)
{
    //Redirect
    CJSON::_freeJSON_VALUE(val);
}
void JSON_NODE::_freeJSON_VALUE(JSON_VALUE& val)
{
    //Redirect
    CJSON::_freeJSON_VALUE(val);
}

void CJSON::_freeJSON_VALUE(JSON_VALUE& val)
{
    switch(val.valType)
    {
    case JVT_ARRAY:
        {
            JSON_ARRAY* pJA = (JSON_ARRAY*)val.pValue;
            ASSERT(pJA);
            _freeJSON_ARRAY(pJA);
        }
        break;

    case JVT_OBJECT:
        {
            JSON_OBJECT* pJO = (JSON_OBJECT*)val.pValue;
            ASSERT(pJO);
            _freeJSON_OBJECT(pJO);
        }
        break;
            
    default:
        break;
    }
}


void CJSON::_describeError(JSON_ERROR* pJError, intptr_t i, LPCTSTR pErrDesc)
{
    //Adds error description to 'pJError' if it's not there already
    //INFO: In other words, it keeps only the first error info
    if(pJError)
    {
        if(pJError->isEmpty())
        {
            pJError->nErrIndex = i;
            pJError->strErrDesc = pErrDesc ? pErrDesc : L("");
            pJError->markFilled();
        }
    }
}


bool JSON_NODE::parseFloat(LPCTSTR pStr, double* pfOutVal)
{
    //Redirect
    return CJSON::parseFloat(pStr, pfOutVal);
}

bool CJSON::parseFloat(LPCTSTR pStr, double* pfOutVal)
{
    //Parse float value from 'pStr' presented in local format
    //'pfOutVal' = if not nullptr, receives value parsed
    //RETURN:
    //		= true if success
    bool bRes = false;

    double fVal = 0.0;

    WCHAR* pp = nullptr;

#ifdef _WIN32
    //Windows specific
    fVal = _tcstod(pStr, &pp);
    
#elif __APPLE__
    //macOS specific
    fVal = strtod(pStr, &pp);
    
#endif
    
    //Did we succeed?
    bRes = pp && *pp == 0;

    if(pfOutVal)
        *pfOutVal = fVal;

    return bRes;
}


bool CJSON::isFloatingPointNumberString(LPCTSTR pStr)
{
    //Checks if 'pStr' contains an floating point number
    //RETURN:
    //		= true if yes
    return parseFloat(pStr);
}

bool JSON_NODE::isIntegerBase10String(LPCTSTR pStr)
{
    //Redirect
    return CJSON::isIntegerBase10String(pStr);
}

bool CJSON::isIntegerBase10String(LPCTSTR pStr)
{
    //Checks if 'pStr' contains an integer
    //RETURN:
    //		= true if yes
    bool bRes = false;

    if(pStr &&
        pStr[0])
    {
        //Assume yes
        bRes = true;

        intptr_t nLn = STRLEN(pStr);
        for(intptr_t i = 0; i < nLn; i++)
        {
            WCHAR z = pStr[i];

            //For first char only
            if(i == 0)
            {
                if(z == '+' ||
                    z == '-')
                {
                    //Good char
                    continue;
                }
            }

            if(z >= '0' &&
                z <= '9')
            {
                //Good char
                continue;
            }

            //Bad char
            bRes = false;
            break;
        }
    }

    return bRes;
}



JSON_NODE_TYPE CJSON::_determineNodeTypeSafe(JSON_VALUE* pVal)
{
    //RETURN:
    //		= Type of node by 'pVal', or
    //		  INFO: Cannot be JNT_ERROR or JNT_NONE, uses JNT_STRING instead
    JSON_NODE_TYPE type = _determineNodeType(pVal);
    if(type == JNT_ERROR ||
        type == JNT_NONE)
    {
        //Use default value
        ASSERT(nullptr);
        type = JNT_STRING;
    }

    return type;
}

JSON_NODE_TYPE JSON_NODE::_determineNodeType(JSON_VALUE* pVal)
{
    //Redirect
    return CJSON::_determineNodeType(pVal);
}

JSON_NODE_TYPE CJSON::_determineNodeType(JSON_VALUE* pVal)
{
    //RETURN:
    //		= Type of node by 'pVal', or
    //		= JNT_ERROR if error
    if(pVal)
    {
        if(pVal->valType == JVT_ARRAY)
            return JNT_ARRAY;
        else if(pVal->valType == JVT_OBJECT)
            return JNT_OBJECT;
        else if(pVal->valType == JVT_NONE)
            return JNT_NONE;
        else if(pVal->valType == JVT_DOUBLE_QUOTED)
        {
            //Just a string
            return JNT_STRING;
        }
        else if(pVal->valType == JVT_PLAIN)
        {
            //Check special cases
            //if(pVal->strValue.Compare(L"null") == 0)
            if(json::JSON_NODE::compareStringsEqual(pVal->strValue, L("null"), true))
                return JNT_NULL;
            //else if(pVal->strValue.Compare(L"true") == 0 ||
            //	pVal->strValue.Compare(L"false") == 0)
            else if(json::JSON_NODE::compareStringsEqual(pVal->strValue, L("true"), true) ||
                json::JSON_NODE::compareStringsEqual(pVal->strValue, L("false"), true))
                return JNT_BOOLEAN;

            //See if it's an integer or a floating point number
            if(isIntegerBase10String(pVal->strValue.c_str()))
                return JNT_INTEGER;
            else if(isFloatingPointNumberString(pVal->strValue.c_str()))
                return JNT_FLOAT;
            else
            {
                //Else just a string
                return JNT_STRING;
            }
        }
    }

    return JNT_ERROR;
}


JSON_NODE_TYPE JSON_NODE::findNodeByIndex(intptr_t nIndex, JSON_NODE* pJNodeFound)
{
    //Look for the node in this node with the 'nIndex'
    //INFO: This node must be an object or array node
    //'nIndex' = node zero-based index (use JSON_NODE::getNodeCount() to get number of nodes)
    //'pJNodeFound' = if not nullptr, receives the data for the node found
    //RETURN:
    //		= Node type if found, or
    //		= JNT_ERROR if error in search parameters
    JSON_NODE_TYPE resType = JNT_ERROR;

    if(isNodeSet())
    {
        ASSERT(pVal);
        if(pVal)
        {
            //Check node type
            if(pVal->valType == JVT_OBJECT)
            {
                JSON_OBJECT* pJO = (JSON_OBJECT*)pVal->pValue;
                ASSERT(pJO);
                if(pJO)
                {
                    if(nIndex >= 0 &&
                        nIndex < (intptr_t)pJO->arrObjElmts.size())
                    {
                        //Got it
                        JSON_OBJECT_ELEMENT* pJOE = &pJO->arrObjElmts[nIndex];

                        //Type
                        resType = CJSON::_determineNodeTypeSafe(&pJOE->val);
                        ASSERT(resType != JNT_NONE && resType != JNT_ERROR);

                        if(pJNodeFound)
                        {
                            pJNodeFound->typeNode = resType;
                            pJNodeFound->strName = pJOE->strName;
                            pJNodeFound->pVal = &pJOE->val;

                            pJNodeFound->pJSONData = pJSONData;
                        }
                    }
                }
            }
            else if(pVal->valType == JVT_ARRAY)
            {
                JSON_ARRAY* pJA = (JSON_ARRAY*)pVal->pValue;
                ASSERT(pJA);
                if(pJA)
                {
                    if(nIndex >= 0 &&
                        nIndex < (intptr_t)pJA->arrArrElmts.size())
                    {
                        //Got it
                        JSON_ARRAY_ELEMENT* pJAE = &pJA->arrArrElmts[nIndex];

                        //Type
                        resType = CJSON::_determineNodeTypeSafe(&pJAE->val);
                        ASSERT(resType != JNT_NONE && resType != JNT_ERROR);

                        if(pJNodeFound)
                        {
                            pJNodeFound->typeNode = resType;
                            pJNodeFound->strName.clear();
                            pJNodeFound->pVal = &pJAE->val;

                            pJNodeFound->pJSONData = pJSONData;
                        }
                    }
                }
            }
        }
    }

    return resType;
}


JSON_NODE_TYPE JSON_NODE::findNodeByName(LPCTSTR pStrName, JSON_NODE* pJNodeFound, bool bCaseSensitive, JSON_SRCH* pJSrch)
{
    //Look for the next node in this node with the name 'pStrName'
    //INFO: Can be called repeatedly if 'JSON_SRCH' is used
    //INFO: This node must be an object node only
    //'pStrName' = node (or element) name to look for (cannot be ""!)
    //'pJNodeFound' = if not nullptr, receives the data for the node found
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //'pJSrch' = if not nullptr, must be used for repeated searches for the same node name (keep calling this method while it succeeds in finding)
    //RETURN:
    //		= Node type if found, or
    //		= JNT_NONE if nothing was found, or
    //		= JNT_ERROR if error in search parameters
    JSON_NODE_TYPE resType = JNT_ERROR;

    if(isNodeSet() &&
        pStrName &&
        pStrName[0])
    {
        ASSERT(pVal);
        if(pVal)
        {
            //Only if it's an object
            if(pVal->valType == JVT_OBJECT)
            {
                JSON_OBJECT* pJO = (JSON_OBJECT*)pVal->pValue;
                if(pJO)
                {
                    //Assume nothing was found
                    resType = JNT_NONE;

                    intptr_t nCntJOs = (intptr_t)pJO->arrObjElmts.size();
                    JSON_OBJECT_ELEMENT* pJOEs = pJO->arrObjElmts.data();

                    intptr_t nFndInd = -1;
                    intptr_t nLnStrName = STRLEN(pStrName);

                    if(bCaseSensitive)
                    {
                        //Case sensitive search
                        for(intptr_t i = pJSrch ? pJSrch->nIndex : 0; i < nCntJOs; i++)
                        {
                            if(nLnStrName == pJOEs[i].strName.size() &&
                                memcmp(pJOEs[i].strName.c_str(), pStrName, nLnStrName * sizeof(WCHAR)) == 0)
                            {
                                //Matched
                                nFndInd = i;
                                break;
                            }
                        }
                    }
                    else
                    {
                        //Case insensitive search
                        for(intptr_t i = pJSrch ? pJSrch->nIndex : 0; i < nCntJOs; i++)
                        {
                            if(JSON_NODE::compareStringsEqual(pJOEs[i].strName.c_str(),
                                                              pJOEs[i].strName.size(),
                                                              pStrName,
                                                              nLnStrName,
                                                              false))
                            {
                                //Matched
                                nFndInd = i;
                                break;
                            }
                        }
                    }

                    //Did we find it?
                    if(nFndInd >= 0 &&
                        nFndInd < nCntJOs)
                    {
                        //Determine node type
                        resType = CJSON::_determineNodeTypeSafe(&pJOEs[nFndInd].val);
                        ASSERT(resType != JNT_NONE && resType != JNT_ERROR);

                        //Do we need to return it?
                        if(pJNodeFound)
                        {
                            //Fill out the node found
                            pJNodeFound->typeNode = resType;
                            pJNodeFound->strName = pJOEs[nFndInd].strName;
                            pJNodeFound->pVal = &pJOEs[nFndInd].val;

                            pJNodeFound->pJSONData = pJSONData;
                        }

                        //Did we have a search struct?
                        if(pJSrch)
                        {
                            //Update index for the next search
                            pJSrch->nIndex = nFndInd + 1;
                        }
                    }
                }
            }
        }
    }

    return resType;
}


JSON_NODE_TYPE JSON_NODE::findNodeByNameAndGetValueAsString(LPCTSTR pStrName, std_wstring* pOutStr, bool bCaseSensitive)
{
    //Look for the first node in this node with the name 'pStrName'
    //INFO: This node must be an object node only
    //INFO: In case of more than one node matching the search, only the first one is retrieved
    //'pStrName' = node (or element) name to look for (cannot be ""!)
    //'pOutStr' = if not nullptr, receives the found node value
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //		= Node type if found, or
    //		= JNT_NONE if nothing was found, or
    //		= JNT_ERROR if error in search parameters, or retrieval
    JSON_NODE jsnNd;
    JSON_NODE_TYPE type = findNodeByName(pStrName, &jsnNd);
    if(type > JNT_NONE)
    {
        //Read as string
        if(!jsnNd.getValueAsString(pOutStr))
        {
            //Failed
            ASSERT(nullptr);
            type = JNT_ERROR;
        }
    }

    return type;
}

JSON_NODE_TYPE JSON_NODE::findNodeByNameAndGetValueAsInt32(LPCTSTR pStrName, int* pOuVal, bool bCaseSensitive)
{
    //Look for the first node in this node with the name 'pStrName'
    //INFO: This node must be an object node only
    //INFO: In case of more than one node matching the search, only the first one is retrieved
    //'pStrName' = node (or element) name to look for (cannot be ""!)
    //'pOuVal' = if not nullptr, receives the found node value
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //        = Node type if found, or
    //        = JNT_NONE if nothing was found, or
    //        = JNT_ERROR if error in search parameters, or retrieval
    JSON_NODE jsnNd;
    JSON_NODE_TYPE type = findNodeByName(pStrName, &jsnNd);
    if(type > JNT_NONE)
    {
        //Read as string
        if(!jsnNd.getValueAsInt32(pOuVal))
        {
            //Failed
            ASSERT(nullptr);
            type = JNT_ERROR;
        }
    }

    return type;
}

JSON_NODE_TYPE JSON_NODE::findNodeByNameAndGetValueAsInt64(LPCTSTR pStrName, int64_t* pOuVal, bool bCaseSensitive)
{
    //Look for the first node in this node with the name 'pStrName'
    //INFO: This node must be an object node only
    //INFO: In case of more than one node matching the search, only the first one is retrieved
    //'pStrName' = node (or element) name to look for (cannot be ""!)
    //'pOuVal' = if not nullptr, receives the found node value
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //        = Node type if found, or
    //        = JNT_NONE if nothing was found, or
    //        = JNT_ERROR if error in search parameters, or retrieval
    JSON_NODE jsnNd;
    JSON_NODE_TYPE type = findNodeByName(pStrName, &jsnNd);
    if(type > JNT_NONE)
    {
        //Read as string
        if(!jsnNd.getValueAsInt64(pOuVal))
        {
            //Failed
            ASSERT(nullptr);
            type = JNT_ERROR;
        }
    }

    return type;
}

JSON_NODE_TYPE JSON_NODE::findNodeByNameAndGetValueAsBool(LPCTSTR pStrName, bool* pOuVal, bool bCaseSensitive)
{
    //Look for the first node in this node with the name 'pStrName'
    //INFO: This node must be an object node only
    //INFO: In case of more than one node matching the search, only the first one is retrieved
    //'pStrName' = node (or element) name to look for (cannot be ""!)
    //'pOuVal' = if not nullptr, receives the found node value
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //        = Node type if found, or
    //        = JNT_NONE if nothing was found, or
    //        = JNT_ERROR if error in search parameters, or retrieval
    JSON_NODE jsnNd;
    JSON_NODE_TYPE type = findNodeByName(pStrName, &jsnNd);
    if(type > JNT_NONE)
    {
        //Read as string
        if(!jsnNd.getValueAsBool(pOuVal))
        {
            //Failed
            ASSERT(nullptr);
            type = JNT_ERROR;
        }
    }

    return type;
}


JSON_NODE_TYPE JSON_NODE::findNodeByIndexAndGetValueAsString(intptr_t nIndex, std_wstring* pOutStr)
{
    //Look for the node in this node with the 'nIndex'
    //INFO: This node must be an object or array node
    //'nIndex' = node zero-based index (use JSON_NODE::getNodeCount() to get number of nodes)
    //'pOutStr' = if not nullptr, receives the found node value
    //RETURN:
    //		= Node type if found, or
    //		= JNT_ERROR if error in search parameters
    JSON_NODE jsnNd;
    JSON_NODE_TYPE type = this->findNodeByIndex(nIndex, &jsnNd);
    if(type > JNT_NONE)
    {
        //Read as string
        if(!jsnNd.getValueAsString(pOutStr))
        {
            //Failed
            ASSERT(nullptr);
            type = JNT_ERROR;
        }
    }

    return type;
}


bool JSON_NODE::setAsRootNode(JSON_DATA* pJSON_Data)
{
    //Set this node as a root node
    //INFO: This will erase all data from this node
    //INFO: Can be called on the node that is not set yet, or on a previous root node only
    //'pJSON_Data' = JSON data holder, or nullptr to reuse existing data holder (if one was previously present)
    //RETURN:
    //		= true if success
    bool bRes = false;

    //Is it OK to change it
    bool bOKToSetIt = true;

    //Is this node set?
    bool bNodeIsSet = this->isNodeSet();
    if(bNodeIsSet)
    {
        if(this->typeNode != JNT_ROOT)
        {
            //Can't make it a root node
            bOKToSetIt = false;
        }
    }

    if(bOKToSetIt)
    {
        //Pick JSON data
        if(!pJSON_Data)
            pJSON_Data = pJSONData;

        ASSERT(pJSON_Data);
        if(pJSON_Data)
        {
            //First create an empty object
            JSON_OBJECT* pJO = new (std::nothrow) JSON_OBJECT;
            if(pJO)
            {
                //Was it set?
                if(bNodeIsSet)
                {
                    //Delete its data first
                    ASSERT(pVal);
                    _freeJSON_VALUE(*pVal);

                    pVal = nullptr;
                }

                //Remember JSON data
                pJSONData = pJSON_Data;

                //Set root data
                pJSON_Data->val.strValue.clear();
                pJSON_Data->val.valType = JVT_OBJECT;
                pJSON_Data->val.pValue = pJO;

                //Set it as an empty object
                this->strName.clear();
                this->typeNode = JNT_ROOT;
                this->pVal = &pJSON_Data->val;

                //Done
                bRes = true;
            }
        }
    }

    return bRes;
}


bool JSON_NODE::setAsEmptyNode(JSON_DATA* pJSON_Data, JSON_NODE_TYPE type)
{
    //Set this node as a empty node
    //INFO: This will erase all data from this node
    //INFO: Can be called on the node that is not set yet, or on a previously used node
    //'pJSON_Data' = JSON data holder, or nullptr to reuse existing data holder (if one was previously present)
    //'type' = type of node to set, can be: JNT_OBJECT or JNT_ARRAY
    //RETURN:
    //		= true if success
    bool bRes = false;

    //Pick JSON data
    if(!pJSON_Data)
        pJSON_Data = pJSONData;

    ASSERT(pJSON_Data);
    if(pJSON_Data)
    {
        //Create new element
        if(type == JNT_OBJECT)
        {
            JSON_OBJECT* pJO = new (std::nothrow) JSON_OBJECT;
            ASSERT(pJO);
            if(pJO)
            {
                //Was node set?
                if(isNodeSet())
                {
                    //Delete its data first
                    ASSERT(pVal);
                    _freeJSON_VALUE(*pVal);

                    pVal = nullptr;
                }

                //Remember JSON data
                pJSONData = pJSON_Data;

                //Set root data
                pJSON_Data->val.strValue.clear();
                pJSON_Data->val.valType = JVT_OBJECT;
                pJSON_Data->val.pValue = pJO;

                //Set it as an empty object
                this->strName.clear();
                this->typeNode = JNT_OBJECT;
                this->pVal = &pJSON_Data->val;

                //Done
                bRes = true;
            }
        }
        else if(type == JNT_ARRAY)
        {
            JSON_ARRAY* pJA = new (std::nothrow) JSON_ARRAY;
            ASSERT(pJA);
            if(pJA)
            {
                //Was node set?
                if(isNodeSet())
                {
                    //Delete its data first
                    ASSERT(pVal);
                    _freeJSON_VALUE(*pVal);

                    pVal = nullptr;
                }

                //Remember JSON data
                pJSONData = pJSON_Data;

                //Set root data
                pJSON_Data->val.strValue.clear();
                pJSON_Data->val.valType = JVT_ARRAY;
                pJSON_Data->val.pValue = pJA;

                //Set it as an empty object
                this->strName.clear();
                this->typeNode = JNT_ARRAY;
                this->pVal = &pJSON_Data->val;

                //Done
                bRes = true;
            }
        }

    }

    return bRes;
}



bool CJSON::_deepCopyJSON_VALUE(JSON_VALUE* pDestV, JSON_VALUE* pSrcV)
{
    //Copy 'pSrcV' into 'pDestV' by erasing previous values in 'pDestV'
    //RETURN:
    //		= true if success
    bool bRes = false;

    if(pDestV &&
        pSrcV)
    {
        //First erase the dest
        if(!pDestV->isEmptyValue())
        {
            CJSON::_freeJSON_VALUE(*pDestV);

            pDestV->pValue = nullptr;
            pDestV->valType = JVT_NONE;
            pDestV->strValue.clear();
        }

        //Then begin copying
        bRes = __copySingleVal(pDestV, pSrcV);
    }

    return bRes;
}

bool CJSON::__copySingleVal(JSON_VALUE* pDestV, JSON_VALUE* pSrcV)
{
    bool bRes = false;
    ASSERT(pDestV);
    ASSERT(pSrcV);

    switch(pSrcV->valType)
    {
    case JVT_NONE:
    case JVT_PLAIN:
    case JVT_DOUBLE_QUOTED:
        {
            pDestV->valType = pSrcV->valType;
            pDestV->strValue = pSrcV->strValue;
            pDestV->pValue = nullptr;

            bRes = true;
        }
        break;

    case JVT_ARRAY:
        {
            pDestV->valType = pSrcV->valType;
            pDestV->strValue = pSrcV->strValue;
            pDestV->pValue = nullptr;

            JSON_ARRAY* pSrcJA = (JSON_ARRAY*)pSrcV->pValue;
            ASSERT(pSrcJA);
            if(pSrcJA)
            {
                JSON_ARRAY* pDestJA = new (std::nothrow) JSON_ARRAY;
                ASSERT(pDestJA);
                if(pDestJA)
                {
                    //Assume success
                    bRes = true;

                    for(intptr_t i = 0; i < (intptr_t)pSrcJA->arrArrElmts.size(); i++)
                    {
                        JSON_ARRAY_ELEMENT jae;
                        if(__copySingleVal(&jae.val, &pSrcJA->arrArrElmts[i].val))
                        {
                            pDestJA->arrArrElmts.push_back(jae);
                        }
                        else
                        {
                            //Error
                            bRes = false;
                            break;
                        }
                    }

                    //Did it go thru?
                    if(bRes)
                    {
                        //Remember the pointer
                        pDestV->pValue = pDestJA;
                    }
                    else
                    {
                        //Free it
                        _freeJSON_ARRAY(pDestJA);
                    }
                }
            }
        }
        break;

    case JVT_OBJECT:
        {
            pDestV->valType = pSrcV->valType;
            pDestV->strValue = pSrcV->strValue;
            pDestV->pValue = nullptr;

            JSON_OBJECT* pSrcJO = (JSON_OBJECT*)pSrcV->pValue;
            ASSERT(pSrcJO);
            if(pSrcJO)
            {
                JSON_OBJECT* pDestJO = new (std::nothrow) JSON_OBJECT;
                ASSERT(pDestJO);
                if(pDestJO)
                {
                    //Assume success
                    bRes = true;

                    intptr_t nCntJOs = (intptr_t)pSrcJO->arrObjElmts.size();
                    JSON_OBJECT_ELEMENT* pJOEs = pSrcJO->arrObjElmts.data();

                    for(intptr_t i = 0; i < nCntJOs; i++)
                    {
                        JSON_OBJECT_ELEMENT joe;
                        if(__copySingleVal(&joe.val, &pJOEs[i].val))
                        {
                            joe.strName = pJOEs[i].strName;

                            pDestJO->arrObjElmts.push_back(joe);
                        }
                        else
                        {
                            //Error
                            bRes = false;
                            break;
                        }
                    }

                    //Did it go thru?
                    if(bRes)
                    {
                        //Remember the pointer
                        pDestV->pValue = pDestJO;
                    }
                    else
                    {
                        //Free it
                        _freeJSON_OBJECT(pDestJO);
                    }
                }
            }
        }
        break;
    }

    return bRes;
}


bool JSON_NODE::addNode(JSON_NODE* pJNode)
{
    //Add new 'pJNode' node to this node (where this node is either an object or array node)
    //INFO: It makes a "deep" copy of 'pJNode'
    //INFO: Can't be used to add nodes from the same JSON data.
    //RETURN:
    //		= true if success
    bool bRes = false;
    ASSERT(pJSONData);

    //Node must be specified and it must have a name
    if(pJNode &&
        pJSONData)
    {
        //Make sure that JSON data is not the same
        if(this->pJSONData != pJNode->pJSONData &&
            pJNode->pJSONData)
        {
            ASSERT(pVal);
            if(pVal->valType == JVT_OBJECT)
            {
                //We must have a name for this node
                if(!pJNode->strName.empty())
                {
                    JSON_OBJECT* pJO = (JSON_OBJECT*)pVal->pValue;
                    ASSERT(pJO);
                    if(pJO)
                    {
                        //Add new
                        JSON_OBJECT_ELEMENT joe;

                        //Copy node name
                        joe.strName = pJNode->strName;

                        //Copy value
                        if(CJSON::_deepCopyJSON_VALUE(&joe.val, pJNode->pVal))
                        {
                            //Add it
                            pJO->arrObjElmts.push_back(joe);

                            //Done
                            bRes = true;
                        }
                    }
                }
            }
            else if(pVal->valType == JVT_ARRAY)
            {
                JSON_ARRAY* pJA = (JSON_ARRAY*)pVal->pValue;
                ASSERT(pJA);
                if(pJA)
                {
                    //Add new
                    JSON_ARRAY_ELEMENT jae;

                    //Copy value
                    if(CJSON::_deepCopyJSON_VALUE(&jae.val, pJNode->pVal))
                    {
                        //Add it
                        pJA->arrArrElmts.push_back(jae);

                        //Done
                        bRes = true;
                    }
                }
            }
            else
            {
                //Can add only to the Array or Object note
                ASSERT(nullptr);
            }
        }
        else
        {
            //Can't use same JSON object
            ASSERT(nullptr);
        }
    }

    return bRes;
}


bool JSON_NODE::_addNode_WithType(LPCTSTR pStrName, JSON_VALUE_TYPE type, LPCTSTR pStrValue)
{
    //Add new node to this node (where this node is either an object or array node)
    //'pStrName' = node name (cannot be empty for objects and not used for arrays)
    //				INFO: JSON specs do not allow duplicate names (this class allows it.)
    //				INFO: JSON specs specifies that names are case-sensitive (this class can treat them as case-insensitive when looking for nodes)
    //						http://tools.ietf.org/html/rfc7158
    //'type' = type to assign to value. Can be JVT_PLAIN or JVT_DOUBLE_QUOTED
    //'pStrValue' = value
    //RETURN:
    //		= true if success
    bool bRes = false;
    ASSERT(pJSONData);

    if(pJSONData)
    {
        //Check allowed types
        if(type == JVT_PLAIN ||
            type == JVT_DOUBLE_QUOTED)
        {
            ASSERT(pVal);
            if(pVal->valType == JVT_OBJECT)
            {
                //Only if we have a name
                if(pStrName &&
                    pStrName[0])
                {
                    JSON_OBJECT* pJO = (JSON_OBJECT*)pVal->pValue;
                    ASSERT(pJO);
                    if(pJO)
                    {
                        //Add new
                        JSON_OBJECT_ELEMENT joe;

                        //Copy node name
                        joe.strName = pStrName;

                        //And value
                        joe.val.valType = type;
                        joe.val.strValue = pStrValue ? pStrValue : L("");
                        joe.val.pValue = nullptr;

                        if(type == JVT_PLAIN)
                        {
                            //Trim value
                            CJSON::Trim(joe.val.strValue);
                        }

                        //Add it
                        pJO->arrObjElmts.push_back(joe);

                        //Done
                        bRes = true;
                    }
                }
            }
            else if(pVal->valType == JVT_ARRAY)
            {
                JSON_ARRAY* pJA = (JSON_ARRAY*)pVal->pValue;
                ASSERT(pJA);
                if(pJA)
                {
                    //Add new
                    JSON_ARRAY_ELEMENT jae;

                    //Set value
                    jae.val.valType = type;
                    jae.val.strValue = pStrValue ? pStrValue : L("");
                    jae.val.pValue = nullptr;

                    if(type == JVT_PLAIN)
                    {
                        //Trim value
                        CJSON::Trim(jae.val.strValue);
                    }

                    //Add it
                    pJA->arrArrElmts.push_back(jae);

                    //Done
                    bRes = true;
                }
            }
            else
            {
                //Can add only to the Array or Object note
                ASSERT(nullptr);
            }
        }
    }

    return bRes;
}

bool JSON_NODE::addNode_String(LPCTSTR pStrName, LPCTSTR pStrValue)
{
    //Add new string node to this node (where this node is either an object or array node)
    //'pStrName' = node name (cannot be empty for objects and not used for arrays)
    //				INFO: JSON specs do not allow duplicate names (this class allows it.)
    //				INFO: JSON specs specifies that names are case-sensitive (this class can treat them as case-insensitive when looking for nodes)
    //						http://tools.ietf.org/html/rfc7158
    //'pStrValue' = value
    //RETURN:
    //		= true if success
    return _addNode_WithType(pStrName, JVT_DOUBLE_QUOTED, pStrValue);
}

bool JSON_NODE::addNode_BOOL(LPCTSTR pStrName, bool bValue)
{
    //Add new boolean node to this node (where this node is either an object or array node)
    //'pStrName' = node name (cannot be empty for objects and not used for arrays)
    //				INFO: JSON specs do not allow duplicate names (this class allows it.)
    //				INFO: JSON specs specifies that names are case-sensitive (this class can treat them as case-insensitive when looking for nodes)
    //						http://tools.ietf.org/html/rfc7158
    //'bValue' = value
    //RETURN:
    //		= true if success

    //And add it
    return _addNode_WithType(pStrName, JVT_PLAIN, bValue ? L("true") : L("false"));
}

bool JSON_NODE::addNode_Bool(LPCTSTR pStrName, bool bValue)
{
    //Add new boolean node to this node (where this node is either an object or array node)
    //'pStrName' = node name (cannot be empty for objects and not used for arrays)
    //				INFO: JSON specs do not allow duplicate names (this class allows it.)
    //				INFO: JSON specs specifies that names are case-sensitive (this class can treat them as case-insensitive when looking for nodes)
    //						http://tools.ietf.org/html/rfc7158
    //'bValue' = value
    //RETURN:
    //		= true if success

    //And add it
    return _addNode_WithType(pStrName, JVT_PLAIN, bValue ? L("true") : L("false"));
}

bool JSON_NODE::addNode_Null(LPCTSTR pStrName)
{
    //Add new null node to this node (where this node is either an object or array node)
    //'pStrName' = node name (cannot be empty for objects and not used for arrays)
    //				INFO: JSON specs do not allow duplicate names (this class allows it.)
    //				INFO: JSON specs specifies that names are case-sensitive (this class can treat them as case-insensitive when looking for nodes)
    //						http://tools.ietf.org/html/rfc7158
    //RETURN:
    //		= true if success

    //And add it
    return _addNode_WithType(pStrName, JVT_PLAIN, L("null"));
}

bool JSON_NODE::addNode_Int(LPCTSTR pStrName, LPCTSTR pStrValue)
{
    //Add new integer node to this node (where this node is either an object or array node)
    //'pStrName' = node name (cannot be empty for objects and not used for arrays)
    //				INFO: JSON specs do not allow duplicate names (this class allows it.)
    //				INFO: JSON specs specifies that names are case-sensitive (this class can treat them as case-insensitive when looking for nodes)
    //						http://tools.ietf.org/html/rfc7158
    //'pStrValue' = value
    //RETURN:
    //		= true if success

    //Check if number
    if(!CJSON::isIntegerBase10String(pStrValue))
        return false;

    //And add it
    return _addNode_WithType(pStrName, JVT_PLAIN, pStrValue);
}

bool JSON_NODE::addNode_Int(LPCTSTR pStrName, int nValue)
{
    //Add new integer node to this node (where this node is either an object or array node)
    //'pStrName' = node name (cannot be empty for objects and not used for arrays)
    //				INFO: JSON specs do not allow duplicate names (this class allows it.)
    //				INFO: JSON specs specifies that names are case-sensitive (this class can treat them as case-insensitive when looking for nodes)
    //						http://tools.ietf.org/html/rfc7158
    //'nValue' = value
    //RETURN:
    //		= true if success

    //And add it
    return _addNode_WithType(pStrName, JVT_PLAIN, CJSON::easyFormat(L("%d"), nValue).c_str());
}

bool JSON_NODE::addNode_Int64(LPCTSTR pStrName, LPCTSTR pStrValue)
{
    //Add new 64-bit integer node to this node (where this node is either an object or array node)
    //'pStrName' = node name (cannot be empty for objects and not used for arrays)
    //				INFO: JSON specs do not allow duplicate names (this class allows it.)
    //				INFO: JSON specs specifies that names are case-sensitive (this class can treat them as case-insensitive when looking for nodes)
    //						http://tools.ietf.org/html/rfc7158
    //'pStrValue' = value
    //RETURN:
    //		= true if success
    return addNode_Int(pStrName, pStrValue);
}

bool JSON_NODE::addNode_Int64(LPCTSTR pStrName, int64_t iiValue)
{
    //Add new 64-bit integer node to this node (where this node is either an object or array node)
    //'pStrName' = node name (cannot be empty for objects and not used for arrays)
    //				INFO: JSON specs do not allow duplicate names (this class allows it.)
    //				INFO: JSON specs specifies that names are case-sensitive (this class can treat them as case-insensitive when looking for nodes)
    //						http://tools.ietf.org/html/rfc7158
    //'nValue' = value
    //RETURN:
    //		= true if success

    //And add it
    return _addNode_WithType(pStrName, JVT_PLAIN, CJSON::easyFormat(L("%lld"), iiValue).c_str());
}


bool JSON_NODE::addNode_Double(LPCTSTR pStrName, LPCTSTR pStrValue)
{
    //Add new floating point value node to this node (where this node is either an object or array node)
    //'pStrName' = node name (cannot be empty for objects and not used for arrays)
    //				INFO: JSON specs do not allow duplicate names (this class allows it.)
    //				INFO: JSON specs specifies that names are case-sensitive (this class can treat them as case-insensitive when looking for nodes)
    //						http://tools.ietf.org/html/rfc7158
    //'pStrValue' = value
    //RETURN:
    //		= true if success

    //Check if number
    if(!CJSON::isFloatingPointNumberString(pStrValue))
        return false;

    //And add it
    return _addNode_WithType(pStrName, JVT_PLAIN, pStrValue);
}

bool JSON_NODE::addNode_Double(LPCTSTR pStrName, double fValue)
{
    //Add new floating point value node to this node (where this node is either an object or array node)
    //'pStrName' = node name (cannot be empty for objects and not used for arrays)
    //				INFO: JSON specs do not allow duplicate names (this class allows it.)
    //				INFO: JSON specs specifies that names are case-sensitive (this class can treat them as case-insensitive when looking for nodes)
    //						http://tools.ietf.org/html/rfc7158
    //'nValue' = value
    //RETURN:
    //		= true if success

    //And add it
    return _addNode_WithType(pStrName, JVT_PLAIN, CJSON::easyFormat(L("%f"), fValue).c_str());
}



intptr_t JSON_NODE::setNodeByName(LPCTSTR pStrName, JSON_NODE* pJNode, bool bCaseSensitive)
{
    //Set node with 'pStrName' in this node to a new value
    //INFO: It makes a "deep" copy of 'pJNode'
    //INFO: Can't be used to set nodes from the same JSON data.
    //INFO: Sets ALL nodes with the matching name, if there's more than one
    //'pStrName' = node name to set (cannot be empty)
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //		= [1 and up) number if nodes set, or
    //		= 0 if no nodes matched the name provided
    //		= -1 if error setting (some elements might have been copied into destination node)
    intptr_t nCntNodesSet = -1;
    ASSERT(pJSONData);

    //Node must be specified and it must have a name
    if(pJNode &&
        pStrName &&
        pStrName[0] &&
        pJSONData)
    {
        //Make sure that JSON data is not the same
        if(this->pJSONData != pJNode->pJSONData &&
            pJNode->pJSONData)
        {
            ASSERT(pVal);
            if(pVal->valType == JVT_OBJECT)
            {
                JSON_OBJECT* pJO = (JSON_OBJECT*)pVal->pValue;
                ASSERT(pJO);
                if(pJO)
                {
                    //Assume success
                    nCntNodesSet = 0;

                    //Start looking for needed nodes
                    for(JSON_SRCH jSrch;;)
                    {
                        //Find next node by name
                        JSON_NODE_TYPE resFN = findNodeByName(pStrName, nullptr, bCaseSensitive, &jSrch);
                        if(resFN > JNT_NONE)
                        {
                            intptr_t nFndInd = jSrch.getIndexFoundAt();
                            if(nFndInd >= 0 &&
                                nFndInd < (intptr_t)pJO->arrObjElmts.size())
                            {
                                //Pick element found
                                JSON_OBJECT_ELEMENT* pJOE = &pJO->arrObjElmts[nFndInd];

                                //First clear the old value
                            //	CJSON::_freeJSON_VALUE(pJOE->val);			//No need to do it -- it will be done by _deepCopyJSON_VALUE()!

                                //And do "deep" copy
                                if(CJSON::_deepCopyJSON_VALUE(&pJOE->val, pJNode->pVal))
                                {
                                    //Count the ones set
                                    if(nCntNodesSet >= 0)
                                        nCntNodesSet++;
                                }
                                else
                                {
                                    //Failed
                                    ASSERT(nullptr);
                                    nCntNodesSet = -1;
                                }
                            }
                            else
                            {
                                //Error
                                ASSERT(nullptr);
                                nCntNodesSet = -1;
                                break;
                            }
                        }
                        else
                        {
                            if(resFN == JNT_ERROR)
                            {
                                //Error
                                ASSERT(nullptr);
                                nCntNodesSet = -1;
                            }

                            break;
                        }
                    }

                }
            }
            else
            {
                //Can be only an Object node
                ASSERT(nullptr);
            }
        }
        else
        {
            //Can't use same JSON object
            ASSERT(nullptr);
        }
    }

    return nCntNodesSet;
}


bool JSON_NODE::setNodeByIndex(intptr_t nIndex, JSON_NODE* pJNode)
{
    //Set node with 'nIndex' in this node to a new value
    //INFO: It makes a "deep" copy of 'pJNode'
    //INFO: Can't be used to set nodes from the same JSON data.
    //'nIndex' = node's 0-based index to set
    //RETURN:
    //		= true if success
    bool bRes = false;
    ASSERT(pJSONData);

    //Node must be specified and valid
    if(pJNode &&
        pJSONData)
    {
        //Make sure that JSON data is not the same
        if(this->pJSONData != pJNode->pJSONData &&
            pJNode->pJSONData)
        {
            ASSERT(pVal);
            if(pVal->valType == JVT_OBJECT)
            {
                JSON_OBJECT* pJO = (JSON_OBJECT*)pVal->pValue;
                ASSERT(pJO);
                if(pJO)
                {
                    //Make sure index is within limits
                    if(nIndex >= 0 &&
                        nIndex < (intptr_t)pJO->arrObjElmts.size())
                    {
                        //Pick element found
                        JSON_OBJECT_ELEMENT* pJOE = &pJO->arrObjElmts[nIndex];

                        //First clear the old value
                    //	CJSON::_freeJSON_VALUE(pJOE->val);			//No need to do it -- it will be done by _deepCopyJSON_VALUE()!

                        //And do "deep" copy
                        if(CJSON::_deepCopyJSON_VALUE(&pJOE->val, pJNode->pVal))
                        {
                            //Done
                            bRes = true;
                        }
                        else
                        {
                            //Failed
                            ASSERT(nullptr);
                        }
                    }
                }
            }
            else if(pVal->valType == JVT_ARRAY)
            {
                JSON_ARRAY* pJA = (JSON_ARRAY*)pVal->pValue;
                ASSERT(pJA);
                if(pJA)
                {
                    //Make sure index is within limits
                    if(nIndex >= 0 &&
                        nIndex < (intptr_t)pJA->arrArrElmts.size())
                    {
                        //Pick element found
                        JSON_ARRAY_ELEMENT* pJAE = &pJA->arrArrElmts[nIndex];

                        //First clear the old value
                    //	CJSON::_freeJSON_VALUE(pJAE->val);			//No need to do it -- it will be done by _deepCopyJSON_VALUE()!

                        //And do "deep" copy
                        if(CJSON::_deepCopyJSON_VALUE(&pJAE->val, pJNode->pVal))
                        {
                            //Done
                            bRes = true;
                        }
                        else
                        {
                            //Failed
                            ASSERT(nullptr);
                        }
                    }
                }
            }
            else
            {
                //Can be only an Object node
                ASSERT(nullptr);
            }
        }
        else
        {
            //Can't use same JSON object
            ASSERT(nullptr);
        }
    }

    return bRes;
}


intptr_t JSON_NODE::_setNodeByName_WithType(LPCTSTR pStrName, JSON_VALUE_TYPE type, LPCTSTR pStrValue, bool bCaseSensitive)
{
    //Set node with 'pStrName' in this node to a new value
    //INFO: This node must be an object.
    //'pStrName' = node name to set (cannot be empty)
    //'type' = type to assign to value. Can be JVT_PLAIN or JVT_DOUBLE_QUOTED
    //'pStrValue' = value
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //		= [1 and up) number if nodes set, or
    //		= 0 if no nodes matched the name provided
    //		= -1 if error setting (some elements might have been copied into destination node)
    intptr_t nCntNodesSet = -1;
    ASSERT(pJSONData);

    if(pJSONData)
    {
        //Check allowed types
        if(type == JVT_PLAIN ||
            type == JVT_DOUBLE_QUOTED)
        {
            ASSERT(pVal);
            if(pVal->valType == JVT_OBJECT)
            {
                //Only if we have a name
                if(pStrName &&
                    pStrName[0])
                {
                    JSON_OBJECT* pJO = (JSON_OBJECT*)pVal->pValue;
                    ASSERT(pJO);
                    if(pJO)
                    {
                        //Assume success
                        nCntNodesSet = 0;

                        //Start looking for needed nodes
                        for(JSON_SRCH jSrch;;)
                        {
                            //Find next node by name
                            JSON_NODE_TYPE resFN = findNodeByName(pStrName, nullptr, bCaseSensitive, &jSrch);
                            if(resFN > JNT_NONE)
                            {
                                intptr_t nFndInd = jSrch.getIndexFoundAt();
                                if(nFndInd >= 0 &&
                                    nFndInd < (intptr_t)pJO->arrObjElmts.size())
                                {
                                    //Pick element found
                                    JSON_OBJECT_ELEMENT* pJOE = &pJO->arrObjElmts[nFndInd];

                                    //First clear the old value
                                    CJSON::_freeJSON_VALUE(pJOE->val);

                                    //And set new simple value
                                    pJOE->strName = pStrName;

                                    //And value
                                    pJOE->val.valType = type;
                                    pJOE->val.strValue = pStrValue ? pStrValue : L("");
                                    pJOE->val.pValue = nullptr;

                                    if(type == JVT_PLAIN)
                                    {
                                        //Trim value
                                        CJSON::Trim(pJOE->val.strValue);
                                    }

                                    //Count the ones set
                                    if(nCntNodesSet >= 0)
                                        nCntNodesSet++;
                                }
                                else
                                {
                                    //Error
                                    ASSERT(nullptr);
                                    nCntNodesSet = -1;
                                    break;
                                }
                            }
                            else
                            {
                                if(resFN == JNT_ERROR)
                                {
                                    //Error
                                    ASSERT(nullptr);
                                    nCntNodesSet = -1;
                                }

                                break;
                            }
                        }

                    }
                }
            }
            else
            {
                //Can add only to the Array or Object note
                ASSERT(nullptr);
            }
        }
    }

    return nCntNodesSet;
}


intptr_t JSON_NODE::setNodeByName_String(LPCTSTR pStrName, LPCTSTR pStrValue, bool bCaseSensitive)
{
    //Set node with 'pStrName' to the 'pStrValue'
    //'pStrName' = node name to set (cannot be empty)
    //'pStrValue' = value to set
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //		= [1 and up) number if nodes set, or
    //		= 0 if no nodes matched the name provided
    //		= -1 if error setting (some elements might have been copied into destination node)
    return _setNodeByName_WithType(pStrName, JVT_DOUBLE_QUOTED, pStrValue, bCaseSensitive);
}

intptr_t JSON_NODE::setNodeByName_BOOL(LPCTSTR pStrName, bool bValue, bool bCaseSensitive)
{
    //Set node with 'pStrName' to the 'bValue'
    //'pStrName' = node name to set (cannot be empty)
    //'bValue' = value to set
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //		= [1 and up) number if nodes set, or
    //		= 0 if no nodes matched the name provided
    //		= -1 if error setting (some elements might have been copied into destination node)
    return _setNodeByName_WithType(pStrName, JVT_PLAIN, bValue ? L("true") : L("false"), bCaseSensitive);
}
intptr_t JSON_NODE::setNodeByName_Bool(LPCTSTR pStrName, bool bValue, bool bCaseSensitive)
{
    //Set node with 'pStrName' to the 'bValue'
    //'pStrName' = node name to set (cannot be empty)
    //'bValue' = value to set
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //		= [1 and up) number if nodes set, or
    //		= 0 if no nodes matched the name provided
    //		= -1 if error setting (some elements might have been copied into destination node)
    return _setNodeByName_WithType(pStrName, JVT_PLAIN, bValue ? L("true") : L("false"), bCaseSensitive);
}

intptr_t JSON_NODE::setNodeByName_Null(LPCTSTR pStrName, bool bCaseSensitive)
{
    //Set node with 'pStrName' to the null value
    //'pStrName' = node name to set (cannot be empty)
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //		= [1 and up) number if nodes set, or
    //		= 0 if no nodes matched the name provided
    //		= -1 if error setting (some elements might have been copied into destination node)
    return _setNodeByName_WithType(pStrName, JVT_PLAIN, L("null"), bCaseSensitive);
}

intptr_t JSON_NODE::setNodeByName_Int(LPCTSTR pStrName, LPCTSTR pStrValue, bool bCaseSensitive)
{
    //Set node with 'pStrName' to the 'pStrValue'
    //'pStrName' = node name to set (cannot be empty)
    //'pStrValue' = value to set -- it must be an intefer
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //		= [1 and up) number if nodes set, or
    //		= 0 if no nodes matched the name provided
    //		= -1 if error setting (some elements might have been copied into destination node)

    //Check if number
    if(!CJSON::isIntegerBase10String(pStrValue))
        return -1;

    //And add it
    return _setNodeByName_WithType(pStrName, JVT_PLAIN, pStrValue, bCaseSensitive);
}

intptr_t JSON_NODE::setNodeByName_Int(LPCTSTR pStrName, int nValue, bool bCaseSensitive)
{
    //Set node with 'pStrName' to the 'pStrValue'
    //'pStrName' = node name to set (cannot be empty)
    //'nValue' = value to set
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //		= [1 and up) number if nodes set, or
    //		= 0 if no nodes matched the name provided
    //		= -1 if error setting (some elements might have been copied into destination node)

    //And add it
    return _setNodeByName_WithType(pStrName, JVT_PLAIN, CJSON::easyFormat(L("%d"), nValue).c_str(), bCaseSensitive);
}

intptr_t JSON_NODE::setNodeByName_Int64(LPCTSTR pStrName, LPCTSTR pStrValue, bool bCaseSensitive)
{
    //Set node with 'pStrName' to the 'pStrValue'
    //'pStrName' = node name to set (cannot be empty)
    //'pStrValue' = value to set -- it must be an integer
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //		= [1 and up) number if nodes set, or
    //		= 0 if no nodes matched the name provided
    //		= -1 if error setting (some elements might have been copied into destination node)
    return setNodeByName_Int(pStrName, pStrValue, bCaseSensitive);
}

intptr_t JSON_NODE::setNodeByName_Int64(LPCTSTR pStrName, int64_t iiValue, bool bCaseSensitive)
{
    //Set node with 'pStrName' to the 'pStrValue'
    //'pStrName' = node name to set (cannot be empty)
    //'iiValue' = value to set
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //		= [1 and up) number if nodes set, or
    //		= 0 if no nodes matched the name provided
    //		= -1 if error setting (some elements might have been copied into destination node)

    //And add it
    return _setNodeByName_WithType(pStrName, JVT_PLAIN, CJSON::easyFormat(L("%lld"), iiValue).c_str(), bCaseSensitive);
}


intptr_t JSON_NODE::setNodeByName_Double(LPCTSTR pStrName, LPCTSTR pStrValue, bool bCaseSensitive)
{
    //Set node with 'pStrName' to the 'pStrValue'
    //'pStrName' = node name to set (cannot be empty)
    //'pStrValue' = value to set -- it must be a floating point value
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //		= [1 and up) number if nodes set, or
    //		= 0 if no nodes matched the name provided
    //		= -1 if error setting (some elements might have been copied into destination node)

    //Check if number
    if(!CJSON::isFloatingPointNumberString(pStrValue))
        return -1;

    //And add it
    return _setNodeByName_WithType(pStrName, JVT_PLAIN, pStrValue, bCaseSensitive);
}

intptr_t JSON_NODE::setNodeByName_Double(LPCTSTR pStrName, double fValue, bool bCaseSensitive)
{
    //Set node with 'pStrName' to the 'pStrValue'
    //'pStrName' = node name to set (cannot be empty)
    //'fValue' = value to set
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //		= [1 and up) number if nodes set, or
    //		= 0 if no nodes matched the name provided
    //		= -1 if error setting (some elements might have been copied into destination node)

    //And add it
    return _setNodeByName_WithType(pStrName, JVT_PLAIN, CJSON::easyFormat(L("%f"), fValue).c_str(), bCaseSensitive);
}






bool JSON_NODE::_setNodeByIndex_WithType(intptr_t nIndex, JSON_VALUE_TYPE type, LPCTSTR pStrValue)
{
    //Set node at 'nIndex' in this node to a new value
    //INFO: This node must be an object or an array.
    //'nIndex' = 0-based index of the node to set
    //'type' = type to assign to value. Can be JVT_PLAIN or JVT_DOUBLE_QUOTED
    //'pStrValue' = value
    //RETURN:
    //		= true if success
    bool bRes = false;
    ASSERT(pJSONData);

    if(pJSONData)
    {
        //Check allowed types
        if(type == JVT_PLAIN ||
            type == JVT_DOUBLE_QUOTED)
        {
            ASSERT(pVal);
            if(pVal->valType == JVT_OBJECT)
            {
                JSON_OBJECT* pJO = (JSON_OBJECT*)pVal->pValue;
                ASSERT(pJO);
                if(pJO)
                {
                    //Make sure index is within limits
                    if(nIndex >= 0 &&
                        nIndex < (intptr_t)pJO->arrObjElmts.size())
                    {
                        //Pick element found
                        JSON_OBJECT_ELEMENT* pJOE = &pJO->arrObjElmts[nIndex];

                        //First clear the old value
                        CJSON::_freeJSON_VALUE(pJOE->val);

                        //And set new simple value
                        pJOE->val.valType = type;
                        pJOE->val.strValue = pStrValue ? pStrValue : L("");
                        pJOE->val.pValue = nullptr;

                        if(type == JVT_PLAIN)
                        {
                            //Trim value
                            CJSON::Trim(pJOE->val.strValue);
                        }

                        //Done
                        bRes = true;
                    }
                }
            }
            else if(pVal->valType == JVT_ARRAY)
            {
                JSON_ARRAY* pJA = (JSON_ARRAY*)pVal->pValue;
                ASSERT(pJA);
                if(pJA)
                {
                    //Make sure index is within limits
                    if(nIndex >= 0 &&
                        nIndex < (intptr_t)pJA->arrArrElmts.size())
                    {
                        //Pick element found
                        JSON_ARRAY_ELEMENT* pJAE = &pJA->arrArrElmts[nIndex];

                        //First clear the old value
                        CJSON::_freeJSON_VALUE(pJAE->val);

                        //And set new simple value
                        pJAE->val.valType = type;
                        pJAE->val.strValue = pStrValue ? pStrValue : L("");
                        pJAE->val.pValue = nullptr;

                        if(type == JVT_PLAIN)
                        {
                            //Trim value
                            CJSON::Trim(pJAE->val.strValue);
                        }

                        //Done
                        bRes = true;
                    }
                }
            }
            else
            {
                //Can add only to the Array or Object note
                ASSERT(nullptr);
            }
        }
    }

    return bRes;
}


bool JSON_NODE::setNodeByIndex_String(intptr_t nIndex, LPCTSTR pStrValue)
{
    //Set node at 'nIndex' in this node to a new value
    //INFO: This node must be an object or an array.
    //'nIndex' = 0-based index of the node to set
    //'pStrValue' = value to set
    //RETURN:
    //		= true if success
    return _setNodeByIndex_WithType(nIndex, JVT_DOUBLE_QUOTED, pStrValue);
}

bool JSON_NODE::setNodeByIndex_BOOL(intptr_t nIndex, bool bValue)
{
    //Set node at 'nIndex' in this node to a new value
    //INFO: This node must be an object or an array.
    //'nIndex' = 0-based index of the node to set
    //'bValue' = value to set
    //RETURN:
    //		= true if success

    return _setNodeByIndex_WithType(nIndex, JVT_PLAIN, bValue ? L("true") : L("false"));
}

bool JSON_NODE::setNodeByIndex_Bool(intptr_t nIndex, bool bValue)
{
    //Set node at 'nIndex' in this node to a new value
    //INFO: This node must be an object or an array.
    //'nIndex' = 0-based index of the node to set
    //'bValue' = value to set
    //RETURN:
    //		= true if success

    return _setNodeByIndex_WithType(nIndex, JVT_PLAIN, bValue ? L("true") : L("false"));
}

bool JSON_NODE::setNodeByIndex_Null(intptr_t nIndex)
{
    //Set node at 'nIndex' in this node to a new value
    //INFO: This node must be an object or an array.
    //'nIndex' = 0-based index of the node to set
    //RETURN:
    //		= true if success

    return _setNodeByIndex_WithType(nIndex, JVT_PLAIN, L("null"));
}

bool JSON_NODE::setNodeByIndex_Int(intptr_t nIndex, LPCTSTR pStrValue)
{
    //Set node at 'nIndex' in this node to a new value
    //INFO: This node must be an object or an array.
    //'nIndex' = 0-based index of the node to set
    //'pStrValue' = value to set -- must be an integer
    //RETURN:
    //		= true if success

    //Check if number
    if(!CJSON::isIntegerBase10String(pStrValue))
        return false;

    //And add it
    return _setNodeByIndex_WithType(nIndex, JVT_PLAIN, pStrValue);
}

bool JSON_NODE::setNodeByIndex_Int(intptr_t nIndex, int nValue)
{
    //Set node at 'nIndex' in this node to a new value
    //INFO: This node must be an object or an array.
    //'nIndex' = 0-based index of the node to set
    //'nValue' = value to set
    //RETURN:
    //		= true if success

    //And add it
    return _setNodeByIndex_WithType(nIndex, JVT_PLAIN, CJSON::easyFormat(L("%d"), nValue).c_str());
}

bool JSON_NODE::setNodeByIndex_Int64(intptr_t nIndex, LPCTSTR pStrValue)
{
    //Set node at 'nIndex' in this node to a new value
    //INFO: This node must be an object or an array.
    //'nIndex' = 0-based index of the node to set
    //'pStrValue' = value to set -- must be an integer
    //RETURN:
    //		= true if success
    return setNodeByIndex_Int(nIndex, pStrValue);
}

bool JSON_NODE::setNodeByIndex_Int64(intptr_t nIndex, int64_t iiValue)
{
    //Set node at 'nIndex' in this node to a new value
    //INFO: This node must be an object or an array.
    //'nIndex' = 0-based index of the node to set
    //'iiValue' = value to set
    //RETURN:
    //		= true if success

    //And add it
    return _setNodeByIndex_WithType(nIndex, JVT_PLAIN, CJSON::easyFormat(L("%lld"), iiValue).c_str());
}


bool JSON_NODE::setNodeByIndex_Double(intptr_t nIndex, LPCTSTR pStrValue)
{
    //Set node at 'nIndex' in this node to a new value
    //INFO: This node must be an object or an array.
    //'nIndex' = 0-based index of the node to set
    //'pStrValue' = value to set -- must be a floating point number
    //RETURN:
    //		= true if success

    //Check if number
    if(!CJSON::isFloatingPointNumberString(pStrValue))
        return false;

    //And add it
    return _setNodeByIndex_WithType(nIndex, JVT_PLAIN, pStrValue);
}

bool JSON_NODE::setNodeByIndex_Double(intptr_t nIndex, double fValue)
{
    //Set node at 'nIndex' in this node to a new value
    //INFO: This node must be an object or an array.
    //'nIndex' = 0-based index of the node to set
    //'fValue' = value to set
    //RETURN:
    //		= true if success

    //And add it
    return _setNodeByIndex_WithType(nIndex, JVT_PLAIN, CJSON::easyFormat(L("%f"), fValue).c_str());
}



intptr_t JSON_NODE::removeNodeByName(LPCTSTR pStrName, bool bCaseSensitive)
{
    //Remove nodes from this node that match the name
    //'pStrName' = node name to remove (cannot be empty)
    //'bCaseSensitive' = true if 'pStrName' should be matched in case-sensitive way, false if not
    //RETURN:
    //		= [1 and up) number if nodes removed, or
    //		= 0 if no nodes matched the name provided
    //		= -1 if error (some elements might have been removed)
    intptr_t nCntDel = -1;

    ASSERT(pJSONData);

    //Node must be specified and it must have a name
    if(pStrName &&
        pStrName[0] &&
        pJSONData)
    {
        ASSERT(pVal);
        if(pVal->valType == JVT_OBJECT)
        {
            JSON_OBJECT* pJO = (JSON_OBJECT*)pVal->pValue;
            ASSERT(pJO);
            if(pJO)
            {
                //Assume success
                nCntDel = 0;

                //Start looking for needed nodes
                for(;;)
                {
                    //Find next node by name
                    JSON_SRCH jSrch;		//Place it here, because we're removing elements from array, so need to start from index 0!
                    JSON_NODE_TYPE resFN = findNodeByName(pStrName, nullptr, bCaseSensitive, &jSrch);
                    if(resFN > JNT_NONE)
                    {
                        intptr_t nFndInd = jSrch.getIndexFoundAt();
                        if(nFndInd >= 0 &&
                            nFndInd < (intptr_t)pJO->arrObjElmts.size())
                        {
                            //Pick element found
                            JSON_OBJECT_ELEMENT* pJOE = &pJO->arrObjElmts[nFndInd];

                            //First clear the old value
                            CJSON::_freeJSON_VALUE(pJOE->val);

                            //Then remove the element itself
                            pJO->arrObjElmts.erase(pJO->arrObjElmts.begin() + nFndInd);

                            //Count the ones deleted
                            if(nCntDel >= 0)
                                nCntDel++;

                        }
                        else
                        {
                            //Error
                            ASSERT(nullptr);
                            nCntDel = -1;
                            break;
                        }
                    }
                    else
                    {
                        if(resFN == JNT_ERROR)
                        {
                            //Error
                            ASSERT(nullptr);
                            nCntDel = -1;
                        }

                        break;
                    }
                }

            }
        }
        else
            ASSERT(nullptr);		//Can be only an object node
    }

    return nCntDel;
}


bool JSON_NODE::removeNodeByIndex(intptr_t nIndex)
{
    //Remove node from this node by the 'nIndex'
    //'nIndex' = node's 0-based index to remove
    //RETURN:
    //		= true if removed OK
    bool bRes = false;
    ASSERT(pJSONData);

    //Node must be specified and valid
    if(pJSONData)
    {
        ASSERT(pVal);
        if(pVal->valType == JVT_OBJECT)
        {
            JSON_OBJECT* pJO = (JSON_OBJECT*)pVal->pValue;
            ASSERT(pJO);
            if(pJO)
            {
                //Make sure index is within limits
                if(nIndex >= 0 &&
                    nIndex < (intptr_t)pJO->arrObjElmts.size())
                {
                    //Pick element found
                    JSON_OBJECT_ELEMENT* pJOE = &pJO->arrObjElmts[nIndex];

                    //First clear the old value
                    CJSON::_freeJSON_VALUE(pJOE->val);

                    //Then remove the element itself
                    pJO->arrObjElmts.erase(pJO->arrObjElmts.begin() + nIndex);

                    //Done
                    bRes = true;
                }
            }
        }
        else if(pVal->valType == JVT_ARRAY)
        {
            JSON_ARRAY* pJA = (JSON_ARRAY*)pVal->pValue;
            ASSERT(pJA);
            if(pJA)
            {
                //Make sure index is within limits
                if(nIndex >= 0 &&
                    nIndex < (intptr_t)pJA->arrArrElmts.size())
                {
                    //Pick element found
                    JSON_ARRAY_ELEMENT* pJAE = &pJA->arrArrElmts[nIndex];

                    //First clear the old value
                    CJSON::_freeJSON_VALUE(pJAE->val);

                    //Then remove the element itself
                    pJA->arrArrElmts.erase(pJA->arrArrElmts.begin() + nIndex);

                    //Done
                    bRes = true;
                }
            }
        }
        else
        {
            //Can be only an Object or Array node
            ASSERT(nullptr);
        }
    }

    return bRes;
}


#ifdef __APPLE__
//macOS specific

int JSON_NODE::_compareStringsEqualNoCase_macOS(const char* pStr1,
                                                intptr_t ncbStrLen1,
                                                const char* pStr2,
                                                intptr_t ncbStrLen2)
{
    //Compare two strings in case-insensitive way
    //INFO: This function takes into account non-English alphabets during comparison.
    //'pStr1' = string 1 pointer (may be 0)
    //'ncbStrLen1' = number of bytes in 'pStr1' to compare, or negative to calculate automatically
    //'pStr2' = string 2 pointer (may be 0)
    //'ncbStrLen2' = number of bytes in 'pStr2' to compare, or negative to calculate automatically
    //RETURN:
    //      = 1 if equal
    //      = 0 if not equal
    //      = -1 if error
    int nResult = -1;
    
    if(ncbStrLen1 < 0)
    {
        ncbStrLen1 = pStr1 ? strlen(pStr1) : 0;
    }

    if(ncbStrLen2 < 0)
    {
        ncbStrLen2 = pStr2 ? strlen(pStr2) : 0;
    }
    
    CFStringRef refStr1 = CFStringCreateWithBytesNoCopy(kCFAllocatorDefault,
                                                        (const UInt8 *)(pStr1 ? pStr1 : ""),
                                                        ncbStrLen1,
                                                        kCFStringEncodingUTF8,
                                                        false,
                                                        kCFAllocatorNull);
    if(refStr1)
    {
        CFStringRef refStr2 = CFStringCreateWithBytesNoCopy(kCFAllocatorDefault,
                                                            (const UInt8 *)(pStr2 ? pStr2 : ""),
                                                            ncbStrLen2,
                                                            kCFStringEncodingUTF8,
                                                            false,
                                                            kCFAllocatorNull);
        if(refStr2)
        {
            CFComparisonResult res = CFStringCompare(refStr1,
                                                     refStr2,
                                                     kCFCompareCaseInsensitive);
            
            nResult = res == kCFCompareEqualTo ? 1 : 0;
            
            CFRelease(refStr2);
            refStr2 = nullptr;
        }
        else
            ASSERT(false);
     
        CFRelease(refStr1);
        refStr1 = nullptr;
    }
    else
        ASSERT(false);
    
    return nResult;
}

#endif


bool JSON_NODE::compareStringsEqual(LPCTSTR pStr1, LPCTSTR pStr2, bool bCaseSensitive)
{
    //RETURN:
    //		= true if strings are equal

    if(pStr1 &&
        pStr2)
    {
#ifdef _WIN32
        //Windows specific
        return CompareStringEx(LOCALE_NAME_SYSTEM_DEFAULT, 
                               bCaseSensitive ? 0 : NORM_IGNORECASE,
                               pStr1, -1,
                               pStr2, -1,
                               nullptr, nullptr, NULL) == CSTR_EQUAL;
        
#elif __APPLE__
        //macOS specific
        bool bRes = false;
        
        if(bCaseSensitive)
        {
            //Use binary comparison
            intptr_t nchLnStr1 = strlen(pStr1);
            intptr_t nchLnStr2 = strlen(pStr2);

            if(nchLnStr1 == nchLnStr2)
            {
                if(memcmp(pStr1, pStr2, nchLnStr1) == 0)
                {
                    bRes = true;
                }
            }
        }
        else
        {
            //Case-insensitive comparison
            //INFO: We'll use the OS to provide locale-specific comparison.
            bRes = _compareStringsEqualNoCase_macOS(pStr1, -1, pStr2, -1) == 1;
        }
        
        return bRes;
        
#endif
    }
    else if(!pStr1 && !pStr2)
    {
        return true;
    }

    return false;
}

bool JSON_NODE::compareStringsEqual(LPCTSTR pStr1, intptr_t nchLn1, LPCTSTR pStr2, intptr_t nchLn2, bool bCaseSensitive)
{
    //'nchLn1' = length of 'pStr1' in WCHARs, or -1 to calculate automatically
    //'nchLn2' = length of 'pStr2' in WCHARs, or -1 to calculate automatically
    //RETURN:
    //		= true if strings are equal

    if(pStr1 &&
        pStr2)
    {
#ifdef _WIN32
        //Windows specific
        
        //INFO: Values must be either negative, or less than or equal to 0x7FFFFFFF
        if(nchLn1 <= 0x7FFFFFFF &&
           nchLn2 <= 0x7FFFFFFF)
        {
            return CompareStringEx(LOCALE_NAME_SYSTEM_DEFAULT,
                                   bCaseSensitive ? 0 : NORM_IGNORECASE,
                                   pStr1, nchLn1 >= 0 ? (int)nchLn1 : -1,
                                   pStr2, nchLn2 >= 0 ? (int)nchLn2 : -1,
                                   nullptr, nullptr, NULL) == CSTR_EQUAL;
        }
        else
        {
            //Strings are too long!
            ASSERT(false);
        }
        
#elif __APPLE__
        //macOS specific
        if(nchLn1 < 0)
        {
            nchLn1 = strlen(pStr1);
        }

        if(nchLn2 < 0)
        {
            nchLn2 = strlen(pStr2);
        }
        
        if(bCaseSensitive)
        {
            if(nchLn1 == nchLn2 &&
               memcmp(pStr1, pStr2, nchLn1) == 0)
            {
                return true;
            }
        }
        else
        {
            //use CFStringCreateWithBytesNoCopy
            return _compareStringsEqualNoCase_macOS(pStr1, nchLn1, pStr2, nchLn2) == 1;
        }
#endif
    }
    else if(!pStr1 && !pStr2)
    {
        return true;
    }

    return false;
}

bool JSON_NODE::compareStringsEqual(std_wstring& str1, std_wstring& str2, bool bCaseSensitive)
{
    //RETURN:
    //		= true if strings are equal
    return compareStringsEqual(str1.c_str(), str1.size(), str2.c_str(), str2.size(), bCaseSensitive);
}

bool JSON_NODE::compareStringsEqual(std_wstring& str1, LPCTSTR pStr2, bool bCaseSensitive)
{
    //RETURN:
    //		= true if strings are equal
    return compareStringsEqual(str1.c_str(), str1.size(), pStr2, -1, bCaseSensitive);
}


#ifdef __APPLE__
//macOS specific

bool JSON_NODE::appendUtf8Char(std_wstring& str,
                                unsigned int z)
{
    //Append 'z' to 'str' as UTF-8 byte sequence
    //RETURN:
    //      = true if success
    //      = false if 'z' is out of range for UTF-8 and nothing was appended
    bool bRes = true;
    
    //    0x00000000 - 0x0000007F:
    //        0xxxxxxx
    //
    //    0x00000080 - 0x000007FF:
    //        110xxxxx 10xxxxxx
    //
    //    0x00000800 - 0x0000FFFF:
    //        1110xxxx 10xxxxxx 10xxxxxx
    //
    //    0x00010000 - 0x001FFFFF:
    //        11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

    if(z != 0)
    {
        if(z <= 0x7f)
        {
            //Just a single char
            str += (char)z;
        }
        else if(z <= 0x7FF)
        {
            //110xxxxx 10xxxxxx
            str += (char)(0xC0 | ((z >> 6) & 0x1F));
            str += (char)(0x80 | (z & 0x3F));
        }
        else if(z <= 0xFFFF)
        {
            //1110xxxx 10xxxxxx 10xxxxxx
            str += (char)(0xE0 | ((z >> 12) & 0xF));
            str += (char)(0x80 | ((z >> 6) & 0x3F));
            str += (char)(0x80 | (z & 0x3F));
        }
        else if(z <= 0x1FFFFF)
        {
            //11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            str += (char)(0xF0 | ((z >> 18) & 0x7));
            str += (char)(0x80 | ((z >> 12) & 0x3F));
            str += (char)(0x80 | ((z >> 6) & 0x3F));
            str += (char)(0x80 | (z & 0x3F));
        }
        else
        {
            //Bad utf-8 sequence
            bRes = false;
        }
    }
    else
    {
        //Can't add a null-char
        bRes = false;
    }

    return bRes;
}


intptr_t JSON_NODE::getUtf8Char(const char* pStr,
                                intptr_t i,
                                intptr_t nLn,
                                unsigned int* pOutChar)
{
    //Get UTF-8 char, and its length, that starts at index 'i' in 'pStr'
    //'pStr' = pointer to the beginning of the string - must be valid!
    //'nLn' = length of 'pStr' in bytes (not including last 0)
    //'pOutChar' = if not 0, the UTF-8 char itself, or 0 if error
    //RETURN:
    //      [1 - up) length of UTF-8 char in bytes,
    //      0 if end-of-file was reached
    //      -1 if error in UTF-8 encoding
    
    //    0x00000000 - 0x0000007F:
    //        0xxxxxxx
    //
    //    0x00000080 - 0x000007FF:
    //        110xxxxx 10xxxxxx
    //
    //    0x00000800 - 0x0000FFFF:
    //        1110xxxx 10xxxxxx 10xxxxxx
    //
    //    0x00010000 - 0x001FFFFF:
    //        11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    
    intptr_t ncbLen = 0;
    unsigned int uiChar = 0;
    
    if(i < nLn)
    {
        //Assume an error
        ncbLen = -1;
        
        unsigned char c = pStr[i], c1, c2, c3;
        
        if(!(c & 0x80))
        {
            ncbLen = 1;
            uiChar = c;
        }
        else if((c & 0xE0) == 0xC0)
        {
            if(i + 2 <= nLn)
            {
                //c        c1
                //110xxxxx 10xxxxxx
                
                c1 = pStr[i + 1];
                if((c1 & 0xC0) == 0x80)
                {
                    ncbLen = 2;
                    uiChar = (c1 & 0x3F) | ((unsigned int)(c & 0x1F) << 6);
                }
            }
        }
        else if((c & 0xF0) == 0xE0)
        {
            if(i + 3 <= nLn)
            {
                //c        c1       c2
                //1110xxxx 10xxxxxx 10xxxxxx
                
                c1 = pStr[i + 1];
                c2 = pStr[i + 2];
                if((c1 & 0xC0) == 0x80 &&
                   (c2 & 0xC0) == 0x80)
                {
                    ncbLen = 3;
                    uiChar = (c2 & 0x3F) |
                             ((unsigned int)(c1 & 0x3F) << 6) |
                             ((unsigned int)(c & 0xF) << 12);
                }
            }
        }
        else if((c & 0xF8) == 0xF0)
        {
            if(i + 4 <= nLn)
            {
                //c        c1       c2       c3
                //11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                
                c1 = pStr[i + 1];
                c2 = pStr[i + 2];
                c3 = pStr[i + 3];
                if((c1 & 0xC0) == 0x80 &&
                   (c2 & 0xC0) == 0x80 &&
                   (c3 & 0xC0) == 0x80)
                {
                    ncbLen = 4;
                    uiChar = (c3 & 0x3F) |
                             ((unsigned int)(c2 & 0x3F) << 6) |
                             ((unsigned int)(c1 & 0x3F) << 12) |
                             ((unsigned int)(c & 0x7) << 18);
                }
            }
        }
    }
    
    if(pOutChar)
        *pOutChar = uiChar;
    
    return ncbLen;
}

#endif




bool CJSON::getStringForUTF8(LPCTSTR pStr, std::string& strOut)
{
    //Convert 'pStr' into UTF-8 string
    //'strOut' = receives encoding string as a sequence of bytes
    //RETURN:
    //		= true if success
    //		= false if error (check CJSON::GetLastError() for info)
    return getStringForEncoding(pStr, JENC_UTF_8, strOut);
}

bool CJSON::getStringForEncoding(LPCTSTR pStr, JSON_ENCODING enc, std::string& strOut
#ifdef _WIN32
                                 , bool* pbOutDataLoss
#endif
                                 )
{
    //Convert 'pStr' into some other encoding
    //'enc' = encoding to convert into
    //'strOut' = receives encoding string as a sequence of bytes
    //'pbOutDataLoss' = if not nullptr, will receive true if data loss occurred during conversion
    //                  INFO: Works only on Windows!
    //RETURN:
    //		= true if success
    //		= false if error -- check CJSON::GetLastError() for info
    bool bRes = false;
    int nOSError = NO_ERROR;

#ifdef _WIN32
    bool bUsedDefault = false;
#endif

    //Free string
    strOut.clear();

    if(pStr &&
        pStr[0])
    {
#ifdef _WIN32
        //Windows specific
        UINT nCodePage;
        
        switch(enc)
        {
            case JENC_ANSI:
                nCodePage = CP_ACP;
                break;
            case JENC_UTF_8:
                nCodePage = CP_UTF8;
                break;
            case JENC_UNICODE_16:
                nCodePage = 1200;
                break;
            case JENC_UNICODE_16BE:
                nCodePage = 1201;
                break;

            default:
                nCodePage = -1;
                break;
        }
        
        if(nCodePage != -1)
        {
            int nchLn = (int)wcslen(pStr);
            
            if(nCodePage == 1200)
            {
                //No conversion necessary
                strOut.assign((const char*)pStr, nchLn * sizeof(WCHAR));

                bRes = true;
            }
            else if(nCodePage == 1201)
            {
                //utf-16 with reversed bytes
                size_t szcb = nchLn * sizeof(WCHAR);
                strOut.resize(szcb);
                
                char* pD = &strOut[0];
                const WCHAR* pS = pStr;
                
                for(size_t i = 0; i < szcb; pS++)
                {
                    WCHAR ch = *pS;
                    pD[i++] = (char)(ch >> 8);
                    pD[i++] = (char)(ch);
                }
                
                bRes = true;
            }
            else
            {
                //Get length needed
                int ncbLen = ::WideCharToMultiByte(nCodePage, 0, pStr, nchLn, 0, 0, nullptr, nullptr);
                if(ncbLen > 0)
                {
                    //Check if code page accepts default char
                    bool bUseDefChar = pbOutDataLoss &&
                        nCodePage != CP_UTF8 &&
                        nCodePage != CP_UTF7;
                    
                    //Reserve mem
                    strOut.resize(ncbLen);
                    
                    BOOL b_UsedDef = FALSE;
                    
                    //Convert
                    const char def[] = "?";
                    if(::WideCharToMultiByte(nCodePage, 0, pStr, nchLn, &strOut[0], ncbLen,
                                             bUseDefChar ? def : nullptr, bUseDefChar ? &b_UsedDef : nullptr) == ncbLen)
                    {
                        //Success
                        bRes = true;
                        
                        bUsedDefault = !!b_UsedDef;
                    }
                    else
                        nOSError = ::GetLastError();
                }
                else
                    nOSError = ::GetLastError();
            }
        }
        else
            nOSError = ERROR_INVALID_PARAMETER;

#elif __APPLE__
        //macOS specific
        
        CFStringEncoding encTo;
        
        switch(enc)
        {
            case JENC_ANSI:
                encTo = kCFStringEncodingWindowsLatin1;
                break;
            case JENC_UTF_8:
                encTo = kCFStringEncodingUTF8;
                break;
            case JENC_UNICODE_16:
                encTo = kCFStringEncodingUTF16;
                break;
            case JENC_UNICODE_16BE:
                encTo = kCFStringEncodingUTF16BE;
                break;

            default:
                encTo = -1;
                break;
        }

        if(encTo != -1)
        {
            CFStringRef refStr = CFStringCreateWithCString(kCFAllocatorDefault,
                                                           pStr ? pStr : "",
                                                           kCFStringEncodingUTF8);
            if(refStr)
            {
                const UInt8 chLoss = '?';
                CFRange range = CFRangeMake(0,  CFStringGetLength(refStr));
                
                //Get required length
                CFIndex ncbSz = 0;
                CFIndex ncbConv = CFStringGetBytes(refStr,
                                                   range,
                                                   encTo,
                                                   chLoss,
                                                   false,
                                                   nullptr,
                                                   0,
                                                   &ncbSz);
                if(ncbConv > 0)
                {
                    strOut.resize(ncbSz);
                    
                    //And convert
                    if(CFStringGetBytes(refStr,
                                        range,
                                        encTo,
                                        chLoss,
                                        false,
                                        (UInt8*)&strOut[0],
                                        strOut.size(),
                                        &ncbSz) == ncbConv)
                    {
                        //Done
                        bRes = true;
                    }
                    else
                        nOSError = ERROR_BAD_FORMAT;
                }
                else
                    nOSError = ERROR_BAD_FORMAT;
             
                CFRelease(refStr);
                refStr = nullptr;
            }
            else
                nOSError = ERROR_OUTOFMEMORY;
        }
        else
            nOSError = ERROR_INVALID_PARAMETER;
#endif
    }
    else
    {
        //Empty string
        bRes = true;
    }

#ifdef _WIN32
    if(pbOutDataLoss)
        *pbOutDataLoss = bUsedDefault;
#endif
    
    CJSON::SetLastError(nOSError);
    return bRes;
}



bool CJSON::getUnicodeStringFromEncoding(const char* pAStr, intptr_t ncbLen, JSON_ENCODING enc, std_wstring* pOutUnicodeStr)
{
    //Convert 'pAStr' encoded sequence into the Unicode string
    //INFO: For Microsoft it is UTF-16, and for Apple it's UTF-8.
    //'pAStr' = Byte sequence to convert
    //'ncbLen' = Length of 'pAStr' in bytes, or -1 if it's a null-terminated string
    //'enc' = Encoding that 'pAStr' was encoded with
    //RETURN:
    //		= true if success, and 'pOutUnicodeStr' received the data (must NOT be nullptr!)
    //		= false if error (use CJSON::GetLastError() for details)
    bool bRes = false;
    int nOSError = NO_ERROR;
    ASSERT(pOutUnicodeStr);
    
    pOutUnicodeStr->clear();

    //Do we have the length
    if(ncbLen < 0)
    {
        ncbLen = 0;
        while(pAStr[ncbLen])
            ncbLen++;
    }

    if(ncbLen)
    {
#ifdef _WIN32
        //Windows specific
        UINT nCodePage;
        
        switch(enc)
        {
            case JENC_ANSI:
                nCodePage = CP_ACP;
                break;
            case JENC_UTF_8:
                nCodePage = CP_UTF8;
                break;
            case JENC_UNICODE_16:
                nCodePage = 1200;
                break;
            case JENC_UNICODE_16BE:
                nCodePage = 1201;
                break;

            default:
                nCodePage = -1;
                break;
        }
        
        if(nCodePage != -1)
        {
            if(nCodePage == 1200)
            {
                //Use as-is
                if((ncbLen % sizeof(WCHAR)) == 0)
                {
                    pOutUnicodeStr->assign((LPCTSTR)pAStr, ncbLen / sizeof(WCHAR));

                    bRes = true;
                }
                else
                    nOSError = ERROR_INVALID_DATA;
            }
            else if(nCodePage == 1201)
            {
                //Reverse byte order
                if((ncbLen % sizeof(WCHAR)) == 0)
                {
                    pOutUnicodeStr->resize(ncbLen / sizeof(WCHAR));
                    
                    const char* pS = (const char*)pAStr;
                    WCHAR* pD = &pOutUnicodeStr->at(0);
                    
                    for(intptr_t i = 0; i < ncbLen; pD++)
                    {
                        WCHAR ch = ((WCHAR)pS[i++] << 8);
                        ch |= pS[i++];
                        
                        *pD = ch;
                    }
                    
                    bRes = true;
                }
                else
                    nOSError = ERROR_INVALID_DATA;
            }
            else
            {
                //See how much data do we need?
                int nchLen = ::MultiByteToWideChar(nCodePage, 0, pAStr, (int)ncbLen, nullptr, 0);
                if(nchLen)
                {
                    WCHAR* pWTxt = new(std::nothrow) WCHAR[nchLen];
                    if(pWTxt)
                    {
                        //Convert
                        if(::MultiByteToWideChar(nCodePage, 0, pAStr, (int)ncbLen, pWTxt, nchLen) == nchLen)
                        {
                            //Set text
                            bRes = true;
                            
                            ASSERT(pOutUnicodeStr);
                            pOutUnicodeStr->assign(pWTxt, nchLen);
                        }
                        else
                            nOSError = ::GetLastError();
                        
                        //Free
                        delete[] pWTxt;
                        pWTxt = nullptr;
                    }
                    else
                        nOSError = ERROR_OUTOFMEMORY;
                }
                else
                    nOSError = ::GetLastError();
            }
        }
        else
            nOSError = ERROR_INVALID_PARAMETER;

#elif __APPLE__
        //macOS specific

        CFStringEncoding encFrom;
        
        switch(enc)
        {
            case JENC_ANSI:
                encFrom = kCFStringEncodingWindowsLatin1;
                break;
            case JENC_UTF_8:
                encFrom = kCFStringEncodingUTF8;
                break;
            case JENC_UNICODE_16:
                encFrom = kCFStringEncodingUTF16;
                break;
            case JENC_UNICODE_16BE:
                encFrom = kCFStringEncodingUTF16BE;
                break;

            default:
                encFrom = -1;
                break;
        }

        if(encFrom != -1)
        {
            CFStringRef refStr = CFStringCreateWithBytesNoCopy(kCFAllocatorDefault,
                                                               (const UInt8*)pAStr,
                                                               ncbLen,
                                                               encFrom,
                                                               false,
                                                               kCFAllocatorNull);
            
            if(refStr)
            {
                //And convert it to utf-8
                const char* pStr = CFStringGetCStringPtr(refStr, kCFStringEncodingUTF8);
                if(pStr)
                {
                    //Simple example
                    pOutUnicodeStr->assign(pStr);
                    
                    bRes = true;
                }
                else
                {
                    //Need to do more work
                    CFIndex nchSize = CFStringGetLength(refStr);     //This is the size if characters
                    if(nchSize > 0)
                    {
                        CFIndex ncbSz = CFStringGetMaximumSizeForEncoding(nchSize,
                                                                          kCFStringEncodingUTF8);
                        if(ncbSz != kCFNotFound)
                        {
                            UInt8* pBuff = new (std::nothrow) UInt8[ncbSz + 1];
                            if(pBuff)
                            {
                                if(CFStringGetCString(refStr,
                                                      (char*)pBuff,
                                                      ncbSz + 1,
                                                      kCFStringEncodingUTF8))
                                {
                                    //Note that 'ncbSz' will most certainly include trailing 0's,
                                    //thus we cannot assume that that is the length of our string!
                                    
                                    //Let's place a safety null there first
                                    pBuff[ncbSz] = 0;
                                    
                                    pOutUnicodeStr->assign((const char*)pBuff);
                                    
                                    bRes = true;
                                }
                                else
                                    nOSError = ERROR_INVALID_DATA;
                             
                                //Free mem
                                delete[] pBuff;
                                pBuff = nullptr;
                            }
                            else
                                nOSError = ERROR_OUTOFMEMORY;
                        }
                        else
                            nOSError = ERROR_INVALID_DATA;
                    }
                    else
                        nOSError = ERROR_INVALID_DATA;
                }
                
                //Free
                CFRelease(refStr);
                refStr = nullptr;
            }
            else
                nOSError = ERROR_INVALID_DATA;
        }
        else
            nOSError = ERROR_INVALID_PARAMETER;
        
#endif
    }
    else
    {
        //Empty string
        pOutUnicodeStr->clear();
        bRes = true;
    }

    CJSON::SetLastError(nOSError);
    return bRes;
}

bool CJSON::convertStringToUnicode(const char* pAStr, intptr_t ncbLen, JSON_ENCODING enc, std_wstring* pOutUnicodeStr)
{
    //Convert 'pAStr' encoded sequence into the Unicode string
    //INFO: For Microsoft it is UTF-16, and for Apple it's UTF-8.
    //'pAStr' = Byte sequence to convert
    //'ncbLen' = Length of 'pAStr' in bytes, or -1 if it's a null-terminated string
    //'enc' = Encoding that 'pAStr' was encoded with
    //RETURN:
    //        = true if success, and 'pOutUnicodeStr' received the data (must NOT be nullptr!)
    //        = false if error (use CJSON::GetLastError() for details)
    return CJSON::getUnicodeStringFromEncoding(pAStr, ncbLen, enc, pOutUnicodeStr);
}

bool CJSON::readFileContents(LPCTSTR pStrFilePath, BYTE** ppOutData, UINT* pncbOutDataSz, UINT ncbSzMaxFileSz)
{
    //Read file contents into a BYTE array
    //'pStrFilePath' = file path
    //'ppOutData' = if not nullptr, receives pointer to the BYTE array (must be removed with delete[]!)
    //'pncbOutDataSz' = if not nullptr, receives the size of 'ppOutData' array in BYTEs
    //'ncbSzMaxFileSz' = if not 0, maximum allowed file size in BYTEs
    //RETURN:
    //		= true if success
    //		= false if failed (check CJSON::GetLastError() for info)
    bool bRes = false;
    int nOSError = NO_ERROR;
    BYTE* pFileData = nullptr;
    UINT ncbSzFileData = 0;

    if(pStrFilePath &&
        pStrFilePath[0])
    {
#ifdef _WIN32
        //Windows specific

        //Open file
        HANDLE hFile = ::CreateFile(pStrFilePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
            OPEN_EXISTING, 0, nullptr);
        if(hFile != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER liSz = {0};
            if(::GetFileSizeEx(hFile, &liSz))
            {
                //Check file size (if needed)
                if(!ncbSzMaxFileSz ||
                    liSz.QuadPart <= ncbSzMaxFileSz)
                {
                    //Reserve mem
                    ncbSzFileData = (UINT)liSz.QuadPart;
                    pFileData = new (std::nothrow) BYTE[ncbSzFileData];
                    if(pFileData)
                    {
                        //Read data
                        DWORD dwcbRead = 0;
                        if(::ReadFile(hFile, pFileData, ncbSzFileData, &dwcbRead, nullptr))
                        {
                            if(ncbSzFileData == dwcbRead)
                            {
                                //Got!
                                bRes = true;
                            }
                            else
                            {
                                //Bad size read
                                nOSError = ERROR_BAD_LENGTH;
                            }
                        }
                        else
                        {
                            //Error
                            nOSError = ::GetLastError();
                        }
                    }
                    else
                    {
                        //Memory failed
                        nOSError = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
                else
                {
                    //File is too large
                    nOSError = ERROR_FILE_TOO_LARGE;
                }
            }
            else
            {
                //Error
                nOSError = ::GetLastError();
            }

            //Close file
            VERIFY(::CloseHandle(hFile));
        }
        else
            nOSError = ::GetLastError();
        
#elif __APPLE__
        //macOS specific

        //Read file contents
        FILE* pFile = fopen(pStrFilePath, "rb");
        if(pFile)
        {
            //Get file size
            if(fseek(pFile, 0, SEEK_END) == 0)
            {
                long ncbFileSz = ftell(pFile);
                if(ncbFileSz >= 0)
                {
                    if(fseek(pFile, 0, SEEK_SET) == 0)
                    {
                        //Check file size (if needed)
                        if(!ncbSzMaxFileSz ||
                           ncbFileSz <= ncbSzMaxFileSz)
                        {
                            //Reserve mem
                            pFileData = new (std::nothrow) BYTE[ncbFileSz];
                            if(pFileData)
                            {
                                size_t szcbRead =
                                fread(pFileData, sizeof(char), ncbFileSz, pFile);
                                
                                if(szcbRead == ncbFileSz)
                                {
                                    //Success
                                    bRes = true;
                                    
                                    ncbSzFileData = (UINT)ncbFileSz;
                                }
                                else
                                {
                                    //Bad size
                                    nOSError = EAGAIN;
                                }
                            }
                            else
                                nOSError = ENOMEM;
                        }
                        else
                            nOSError = EFBIG;
                    }
                    else
                        nOSError = errno;
                }
                else
                    nOSError = errno;
            }
            else
                nOSError = errno;
            
            //Close file
            fclose(pFile);
        }
        else
            nOSError = errno;
#endif
    }
    else
        nOSError = ERROR_INVALID_PARAMETER;

    //See if we failed, or don't need the data returned
    if(!bRes ||
        !ppOutData)
    {
        //Free mem
        if(pFileData)
        {
            delete[] pFileData;
            pFileData = nullptr;
        }

        ncbSzFileData = 0;
    }

    if(ppOutData)
        *ppOutData = pFileData;
    if(pncbOutDataSz)
        *pncbOutDataSz = ncbSzFileData;

    CJSON::SetLastError(nOSError);
    return bRes;
}



bool CJSON::readFileContentsAsString(LPCTSTR pStrFilePath, std_wstring* pOutStr, UINT ncbSzMaxFileSz)
{
    //Read file contents into a string
    //INFO: Takes into account BOMs for text file encodings
    //'pStrFilePath' = file path
    //'pOutStr' = if not nullptr, receives the data as a string
    //'ncbSzMaxFileSz' = if not 0, maximum allowed file size in BYTEs
    //RETURN:
    //		= true if success
    //		= false if failed (check CJSON::GetLastError() for info)
    bool bRes = false;
    int nOSError = NO_ERROR;

    //Read file as BYTE string first
    BYTE* pFileData = nullptr;
    UINT ncbSzFileData = 0;
    if(CJSON::readFileContents(pStrFilePath, &pFileData, &ncbSzFileData, ncbSzMaxFileSz))
    {
        int ncbSzBOM;
        JSON_ENCODING enc;

        //Look for BOMs
        if(ncbSzFileData >= 3 &&
            pFileData[0] == 0xef &&
            pFileData[1] == 0xbb &&
            pFileData[2] == 0xbf)
        {
            //UTF-8
            ncbSzBOM = 3;
            enc = JENC_UTF_8;
        }
        else if(ncbSzFileData >= 2 &&
            pFileData[0] == 0xfe &&
            pFileData[1] == 0xff)
        {
            //UTF-16 BE
            ncbSzBOM = 2;
            enc = JENC_UNICODE_16BE;
        }
        else if(ncbSzFileData >= 2 &&
            pFileData[0] == 0xff &&
            pFileData[1] == 0xfe)
        {
            //UTF-16 LE
            ncbSzBOM = 2;
            enc = JENC_UNICODE_16;
        }
        else
        {
            //Treat as ASCII
            ncbSzBOM = 0;
            enc = JENC_ANSI;
        }
        
        
        std_wstring strDummy;
        if(CJSON::convertStringToUnicode((const char *)(pFileData + ncbSzBOM),
                                         ncbSzFileData - ncbSzBOM,
                                         enc,
                                         pOutStr ? pOutStr : &strDummy))
        {
            //Done
            bRes = true;
        }
        else
            nOSError = CJSON::GetLastError();

        
    }
    else
        nOSError = CJSON::GetLastError();

    //Free mem
    if(pFileData)
    {
        delete[] pFileData;
        pFileData = nullptr;
    }

    //Only if failed, otherwise it will be set prior
    if(!bRes &&
        pOutStr)
    {
        pOutStr->clear();
    }

    CJSON::SetLastError(nOSError);
    return bRes;
}



bool CJSON::writeFileContents(LPCTSTR pStrFilePath, const BYTE* pData, size_t ncbDataSz, const BYTE* pBOMData, size_t ncbBOMSz)
{
    //Write file contents from a BYTE array
    //'pStrFilePath' = file path
    //'pData' = if not nullptr, pointer to the BYTE array to write to file
    //'ncbDataSz' = size of 'pData' array in BYTEs
    //'pBOMData' = if not nullptr, points to the file BOM to use (for file encodings)
    //'ncbBOMSz' = size of 'pBOMData' in BYTEs
    //RETURN:
    //		= true if success
    //		= false if failed (check CJSON::GetLastError() for info)
    bool bRes = false;
    int nOSError = NO_ERROR;

    if(pStrFilePath &&
        pData)
    {
#ifdef _WIN32
        //Windows specific
        
        //Open file
        HANDLE hFile = ::CreateFile(pStrFilePath, GENERIC_READ | GENERIC_WRITE, 
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
            CREATE_ALWAYS, 0, nullptr);
        if(hFile != INVALID_HANDLE_VALUE)
        {
            DWORD dwcbWrtn;

            //Do we have a BOM
            bool bBOMWrittenOK = true;
            if(pBOMData &&
                ncbBOMSz)
            {
                //Write BOM
                dwcbWrtn = 0;
                if(!::WriteFile(hFile, pBOMData, (DWORD)ncbBOMSz, &dwcbWrtn, nullptr) ||
                    dwcbWrtn != ncbBOMSz)
                {
                    //Writing BOM failed
                    nOSError = ::GetLastError();
                    bBOMWrittenOK = false;
                }
            }

            if(bBOMWrittenOK)
            {
                //Write main data
                dwcbWrtn = 0;
                if(::WriteFile(hFile, pData, (DWORD)ncbDataSz, &dwcbWrtn, nullptr))
                {
                    if(ncbDataSz == dwcbWrtn)
                    {
                        //Done
                        bRes = true;
                    }
                    else
                        nOSError = ERROR_NET_WRITE_FAULT;
                }
                else
                    nOSError = ::GetLastError();
            }

            //Flush buffer
            //INFO: To make sure the file is fully saved on disk
            VERIFY(::FlushFileBuffers(hFile));

            //Close file
            VERIFY(::CloseHandle(hFile));
        }
        else
            nOSError = ::GetLastError();
        
#elif __APPLE__
        //macOS specific

        //Create new file
        FILE* pFile = fopen(pStrFilePath, "wb+");
        if(pFile)
        {
            size_t szcbWrtn;
            
            //Do we have a BOM
            bool bBOMWrittenOK = true;
            if(pBOMData &&
                ncbBOMSz)
            {
                //Write BOM
                szcbWrtn = 0;
                szcbWrtn = fwrite(pBOMData, sizeof(char), ncbBOMSz, pFile);
                
                if(szcbWrtn != ncbBOMSz)
                {
                    //Writing BOM failed
                    nOSError = errno;
                    bBOMWrittenOK = false;
                }
            }

            if(bBOMWrittenOK)
            {
                //Write the file itself
                szcbWrtn = fwrite(pData, sizeof(char), ncbDataSz, pFile);
                if(szcbWrtn == ncbDataSz)
                {
                    if(fflush(pFile) == 0)
                    {
                        //Done
                        bRes = true;
                    }
                    else
                        nOSError = errno;
                }
                else
                    nOSError = errno;
            }
            
            //Close
            fclose(pFile);
        }
        else
            nOSError = errno;
#endif
    }
    else
        nOSError = ERROR_INVALID_PARAMETER;

    CJSON::SetLastError(nOSError);
    return bRes;
}


bool CJSON::writeFileContentsAsString(LPCTSTR pStrFilePath,
                                      std_wstring* pStr,
                                      JSON_ENCODING enc
#ifdef _WIN32
                                      ,
                                      bool bAllowAnyDataLoss,
                                      bool* pbOutDataLoss
#endif
                                      )
{
    //Write string into a file using a specified encoding
    //INFO: It sets specific BOMs for text file encodings
    //'pStrFilePath' = file path
    //'pStr' = string to write
    //'enc' = encoding to write 'pStr' in
    //'bAllowAnyDataLoss' = true to allow to save file if conversion to the 'enc' causes some data loss, false - not to save file if data loss
    //						INFO: Used only for lossy encoding on Windows.
    //'pbOutDataLoss' = if not nullptr, receives true if saving file caused some data loss due to code page conversion.
    //                      INFO: Used only on Windows/
    //RETURN:
    //		= true if success
    //		= false if failed (check CJSON::GetLastError() for info)
    bool bRes = false;
    int nOSError = NO_ERROR;

#ifdef _WIN32
//Windows specific
    bool bDataLoss = false;
#endif

    if(pStrFilePath &&
       pStrFilePath[0] &&
        pStr)
    {
        //Convert to requested encoding first
        std::string strA;
        if(CJSON::getStringForEncoding(pStr->c_str(),
                                       enc,
                                       strA
#ifdef _WIN32
//Windows specific
                                       , &bDataLoss
#endif
                                       ))
        {
            bool bContinue;
            
#ifdef _WIN32
            //Windows specific
            bContinue = !bAllowAnyDataLoss || !bDataLoss;
#else
            //macOS specific
            bContinue = true;
#endif
            if(bContinue)
            {
                //See what BOM we need to write
                size_t szcbBomSz;
                const BYTE* pBOM;
                
                switch(enc)
                {
                    case JENC_ANSI:
                    {
                        pBOM = (const BYTE*)"";
                        szcbBomSz = 0;
                    }
                    break;
                        
                    case JENC_UTF_8:
                    {
                        static BYTE pBOM_utf8[] = {0xef, 0xbb, 0xbf};
                        
                        pBOM = pBOM_utf8;
                        szcbBomSz = sizeof(pBOM_utf8);
                    }
                    break;
                        
                    case JENC_UNICODE_16:
                    {
                        static BYTE pBOM_utf_16[] = {0xff, 0xfe};
                        
                        pBOM = pBOM_utf_16;
                        szcbBomSz = sizeof(pBOM_utf_16);
                    }
                    break;
                    
                    case JENC_UNICODE_16BE:
                    {
                        static BYTE pBOM_utf_16be[] = {0xfe, 0xff};
                        
                        pBOM = pBOM_utf_16be;
                        szcbBomSz = sizeof(pBOM_utf_16be);
                    }
                    break;

                    default:
                        //Bad encoding
                        pBOM = nullptr;
                        szcbBomSz = 0;
                        break;
                }

                if(pBOM)
                {
                    //Write to file
                    if(writeFileContents(pStrFilePath,
                                         (const BYTE*)strA.data(),
                                         strA.size(),
                                         pBOM,
                                         szcbBomSz))
                    {
                        //Done
                        bRes = true;
                        
                    }
                    else
                        nOSError = CJSON::GetLastError();
                }
                else
                    nOSError = EFAULT;
            }
            else
                nOSError = EINTR;
        }
        else
        {
            //Failed to convert
            nOSError = CJSON::GetLastError();
        }
    }
    else
        nOSError = ERROR_INVALID_PARAMETER;

#ifdef _WIN32
    //Windows specific
    if(pbOutDataLoss)
        *pbOutDataLoss = bDataLoss;
#endif

    CJSON::SetLastError(nOSError);
    return bRes;
}



};
