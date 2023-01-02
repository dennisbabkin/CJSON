# CJSON
*Simple C++ class to create/parse/modify JSON data*

I initially wrote this class for Windows, and now modified it to work under macOS. Note that it is not a cross-platform implementation because C++ does not support proper handling of UTF encodings. (Although this class will probably work for iOS as well.)

## Features

- Create new JSON data and save it to a file.
- Read JSON data from a file or from memory.
- Add/modify/delete existing JSON nodes.
- Support for non-ASCII encodings, such as: UTF-8, UTF-16, UTF-16 (big endian.)
- One simple class without any dependencies other than C++'s STL library for string and array handling.

I wasn't really strictly following JSON specification. I made it do what I needed it to do. But if you want to modify it to follow the specs word-for-word, you're welcome to do that.

## Legal

I provide this code as-is, without any implied liability. There's also no warranty. If you find any bugs, please find what is wrong and post your findings. Otherwise, if you include this class in your code and it breaks something, this is entirely up to you.

# Examples of Use

I'll explain this in code:

```
#include <iostream>
#include <string.h>

#ifdef _WIN32
//Microsoft-specific
#include <Windows.h>
#include <wchar.h>
#endif

#include "JSON.h"



#ifdef _WIN32
//Microsoft-specific
int wmain(int argc, wchar_t *argv[ ])
#else
int main(int argc, char *argv[])
#endif
{
    if(argc > 1)
    {
        //Example of how to create a JSON file
        DynamicallyCreateJsonFile(argv[1]);

        //Example of how to read a JSON file
        LoadJsonFromFile(argv[1]);
    }

    return 0;
}
```

To create the following JSON markup:

```
{
   "employees": {
      "employee": {
         "Surname Name": "Doe",
         "Given Name": "John",
         "Age": 25,
         "Married": true,
         "Currency": "$",
         "Grades": [41, 67]
      },
      "employee": {
         "Surname Name": "Cunningham",
         "Given Name": "Archie",
         "Age": 29,
         "Married": true,
         "Currency": "£",
         "Grades": []
      },
      "pracownik": {
         "Surname Name": "Wójcik",
         "Given Name": "Małgorzata",
         "Age": 44,
         "Married": true,
         "Currency": "€",
         "Grades": [34, 0, 69, 24, 78]
      },
      "verkamaður": {
         "Surname Name": "Guðmundsdóttir",
         "Given Name": "Heiðar",
         "Age": 21,
         "Married": false,
         "Currency": "kr",
         "Grades": [58, 62, 64]
      },
      "ワーカー": {
         "Surname Name": "大谷",
         "Given Name": "翔平",
         "Age": 28,
         "Married": false,
         "Currency": "¥‎",
         "Grades": [5, 45]
      }
   }
}
```

Use:

```
void DynamicallyCreateJsonFile(LPCTSTR pJsonFilePath)
{
    //Create a JSON file dynamically
    //'pJsonFilePath' = file name to save the JSON to

    //First create a root node
    json::JSON_NODE jRoot;
    json::JSON_DATA jDataRoot;

    if(jRoot.setAsRootNode(&jDataRoot))
    {
        //Add an object to it
        json::JSON_DATA jDataNode1;
        json::JSON_NODE jNodeObj(&jDataNode1, L("employees"), json::JNT_OBJECT);

        //Elements to add to the object
        static const struct
        {
            LPCTSTR pCategoryName;
            LPCTSTR pSurname;
            LPCTSTR pGivenName;
            LPCTSTR pCurrencySymbol;
            int age;
            bool married;
            int nCountGrades;
        }
        kEmployees[] = {
            { L("employee"),   L("Doe"),              L("John"),         L("$"),    25, true,  2, },
            { L("employee"),   L("Cunningham"),       L("Archie"),       L("£"),    29, true,  0, },
            { L("pracownik"),  L("Wójcik"),           L("Małgorzata"),   L("€"),    44, true,  5, },
            { L("verkamaður"), L("Guðmundsdóttir"),   L("Heiðar"),       L("kr"),   21, false, 3, },
            { L("ワーカー"),    L("大谷"),             L("翔平"),          L("¥‎"),    28, false, 2, },
        };

        //Add elements to the object
        for(size_t i = 0; i < sizeof(kEmployees) / sizeof(kEmployees[0]); i++)
        {
            json::JSON_DATA jDataNode;
            json::JSON_NODE jNode(&jDataNode, kEmployees[i].pCategoryName, json::JNT_OBJECT);

            jNode.addNode_String(L("Surname Name"), kEmployees[i].pSurname);
            jNode.addNode_String(L("Given Name"), kEmployees[i].pGivenName);
            jNode.addNode_Int(L("Age"), kEmployees[i].age);
            jNode.addNode_Bool(L("Married"), kEmployees[i].married);
            jNode.addNode_String(L("Currency"), kEmployees[i].pCurrencySymbol);

            //Add random grades as an array
            json::JSON_DATA jDataNodeArr;
            json::JSON_NODE jNodeArr(&jDataNodeArr, L("Grades"), json::JNT_ARRAY);

            int nCntGrades = kEmployees[i].nCountGrades;
            for(int g = 0; g < nCntGrades; g++)
            {
                jNodeArr.addNode_Int(nullptr, rand() % 100);
            }

            jNode.addNode(&jNodeArr);


            //Add this node to the object
            if(!jNodeObj.addNode(&jNode))
            {
                //Failed
                assert(false);
            }
        }

        //Add object to the root
        if(jRoot.addNode(&jNodeObj))
        {
            //Example of how to update one specific node (if we need to)
            UpdateNode(jNodeObj, L("ワーカー"), L("Age"));


            //Format JSON into human-readable form
            json::JSON_FORMATTING fmt;
            fmt.bHumanReadable = true;
            fmt.spacesType = json::JSP_USE_SPACES;

            std_wstring strJson;
            if(jDataRoot.toString(&fmt, &strJson))
            {
                //And save it to a text file
                if(!json::CJSON::writeFileContentsAsString(pJsonFilePath, &strJson))
                {
                    //Failed to write file
                    printf("ERROR: %d failed to save file\n", json::CJSON::GetLastError());
                    assert(false);
                }
            }
            else
                assert(false);
        }
        else
            assert(false);
    }
}
```

And, if you need to load a JSON markup from a text file and to parse it so that you can work with it:

```
void LoadJsonFromFile(LPCTSTR pJsonFilePath)
{
    //Load JSON from a file and parse it
    //'pJsonFilePath' = JSON file to load

    //Read file into a text string
    std_wstring strJSON;
    if(json::CJSON::readFileContentsAsString(pJsonFilePath,
        &strJSON,
        0x100000))
    {
        //Parse JSON string
        json::JSON_DATA jData;
        json::JSON_ERROR jErr;
        if(json::CJSON::parseJSON(strJSON.c_str(), jData, &jErr) == 1)
        {
            //Find a root
            json::JSON_NODE jRoot;
            if(jData.getRootNode(&jRoot))
            {
                //Get first child node
                json::JSON_NODE jHostNode;
                if(jRoot.findNodeByIndex(0, &jHostNode) > json::JSON_NODE_TYPE::JNT_NONE)
                {
                    //Find the age of a specific employee: Archie Cunningham
                    json::JSON_SRCH jSearch;
                    json::JSON_NODE jNode;

                    for(;;)
                    {
                        //Search for a specific node
                        json::JSON_NODE_TYPE nodeType =
                            jHostNode.findNodeByName(L("employee"),
                                &jNode,
                                false,
                                &jSearch);

                        if(nodeType == json::JNT_OBJECT)
                        {
                            //Get its value
                            std_wstring strSurname;
                            if(jNode.findNodeByNameAndGetValueAsString(L("Surname Name"),
                                &strSurname,
                                false) == json::JSON_NODE_TYPE::JNT_STRING)
                            {
                                //See if this is our employee?
                                //INFO: For simplicity of this example, we'll use just the last name
#ifdef _WIN32
                                //Windows-specific
                                bool bNeededEmployee = _tcsicmp(strSurname.c_str(), L("Cunningham")) == 0;
#else
                                //Apple specific
                                bool bNeededEmployee = strcasecmp(strSurname.c_str(), L("Cunningham")) == 0;
#endif
                                if(bNeededEmployee)
                                {
                                    //Found it, get his age
                                    int nAge;
                                    if(jNode.findNodeByNameAndGetValueAsInt32(L("Age"), &nAge, false) > json::JSON_NODE_TYPE::JNT_NONE)
                                    {
                                        printf("Age is %d\n", nAge);
                                    }

                                    break;
                                }
                            }
                        }
                        else if(nodeType <= json::JNT_NONE)
                        {
                            //Done with the search - check that we didn't fail
                            assert(nodeType == json::JNT_NONE);
                            break;
                        }
                    }


                    //Example of how to remove a node by its name
                    if(jHostNode.removeNodeByName(L("verkamaður")) <= 0)
                    {
                        //Error
                        assert(false);
                    }


                    //Example of how to add a new node
                    json::JSON_DATA jDataNewNode;
                    json::JSON_NODE jNewNode(&jDataNewNode,
                        L("New Node"),
                        json::JSON_NODE_TYPE::JNT_OBJECT);

                    jNewNode.addNode_String(L("Name1"), L("Value"));
                    jNewNode.addNode_Double(L("Name2"), 3.14);

                    if(!jRoot.addNode(&jNewNode))
                    {
                        assert(false);
                    }


                    //And print the resulting JSON as a human-readable markup
                    //INFO: We'll go with default formatting. Otherwise, fill out the 1st parameter for toString()...
                    std_wstring strJSON;
                    if(jData.toString(nullptr, &strJSON))
                    {
#ifdef _WIN32
                        _tprintf(L("\n=========\n%s\n"), strJSON.c_str());
#else
                        printf(L("\n=========\n%s\n"), strJSON.c_str());
#endif
                    }
                    else
                        assert(false);
                }
                else
                    assert(false);
            }
            else
                assert(false);
        }
        else
        {
            //Failed to parse JSON file - output why
#ifdef _WIN32
            //Microsoft-specific
            _tprintf(L("Failed to parse JSON file:\n"
                "%s\n"
                "'%s' at offset: %Id")
                ,
                pJsonFilePath,
                jErr.strErrDesc.c_str(),
                jErr.nErrIndex);
#else
            //Apple-specific
            printf(L("Failed to parse JSON file:\n"
                "%s\n"
                "'%s' at offset: %ld")
                ,
                pJsonFilePath,
                jErr.strErrDesc.c_str(),
                jErr.nErrIndex);
#endif

            assert(false);
        }
    }
    else
        assert(false);
}
```

Finally, the helper function used in the code above:

```
void UpdateNode(json::JSON_NODE& jNodeObj,
    LPCTSTR pUpdateNodeName,
    LPCTSTR pValueName)
{
    //Find existing node (we'll be fine with just the first object found)
    json::JSON_NODE jNodeChild;
    if(jNodeObj.findNodeByName(pUpdateNodeName, &jNodeChild) > json::JSON_NODE_TYPE::JNT_NONE)
    {
        //Find specific name=value pair in that child node
        json::JSON_NODE jNodeAge;
        if(jNodeChild.findNodeByName(pValueName, &jNodeAge) == json::JSON_NODE_TYPE::JNT_INTEGER)
        {
            //For this example we will take the value and decrement it
            int vAge;
            if(jNodeAge.getValueAsInt32(&vAge))
            {
                vAge--;

                if(jNodeChild.setNodeByName_Int(pValueName, vAge) <= 0)
                {
                    assert(false);
                }
            }
            else
                assert(false);
        }
        else
            assert(false);
    }
    else
        assert(false);
}

```
