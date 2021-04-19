#include <StdInc.h>
#include <charconv>
#include <type_traits>
#undef snprintf

#include <lua/LuaBasic.h>
#include "CValue.h"

bool IsSpecialStringInJSON(std::string_view str)
{
    return str.length() > 3 && str[0] == '^' && str[1] != '^' && str[2] == '^';
}

auto GetGame()
{
#ifdef MTA_CLIENT
    return g_pClientGame;
#else
    return g_pGame;
#endif
}

auto GetScriptDebugging()
{
    return GetGame()->GetScriptDebugging();
}

void LogUnableToPacketize(const char* szMessage)
{
#if 0 /* todo */
    if (m_strFilename.length() > 0)
        GetScriptDebugging()->LogWarning(NULL, "%s:%d: %s\n", ConformResourcePath(m_strFilename.c_str()).c_str(), m_iLine, szMessage);
    else
#endif
        GetScriptDebugging()->LogWarning(NULL, "Unknown: %s\n", szMessage);
}

namespace lua
{


CValue::CValue(lua_State* L, int idx, LuaCopiedValuesMap& copiedTables)
{
    switch (lua_type(L, idx))
    {
    case LUA_TNUMBER:
        m_value.emplace<Number>(lua_tonumber(L, idx));
        break;
    case LUA_TSTRING:
    {
        size_t len;
        const char* value = lua_tolstring(L, idx, &len);
        m_value.emplace<String>(value, len);
        break;
    }
    case LUA_TBOOLEAN:
        m_value.emplace<Bool>(lua_toboolean(L, idx));
        break;
    case LUA_TUSERDATA:
        m_value.emplace<UserData>(*static_cast<UserData*>(lua_touserdata(L, idx)));
        break;
    case LUA_TLIGHTUSERDATA:
        m_value.emplace<UserData>(reinterpret_cast<UserData>(lua_touserdata(L, idx)));
        break;
    case LUA_TTABLE:
    {      
        if (const auto it = copiedTables.find(lua_topointer(L, idx)); it == copiedTables.end()) /* copied yet? */
        {
            /* no, so copy and store it */
            LUA_CHECKSTACK(L, 2);

            lua_pushnil(L); /* first key */
            if (idx < 0) /* correct the table index (because of the above push) */
                idx--;

            /* TODO: Preallocate table in a sensible way.. Maybe iter thru all values? */
            auto& table = m_value.emplace<OwnedTable>(std::make_shared<Table>());
            // table->SetArrKeysCount(lua_objlen(L, -1)); TODO Somehow make this work... 

            copiedTables[lua_topointer(L, idx)] = table; /* Mark as copied before copying others */
            while (lua_next(L, idx))
            {
                CValue k = { L, -2, copiedTables };
                CValue v = { L, -1, copiedTables };
                table->emplace_back(std::move(k), std::move(v));

                /* removes 'value'; keeps 'key' for next iteration */
                lua_pop(L, 1);
            }
        }
        else
        {
            /* copied already, just store a ref to it */
            m_value.emplace<TableRef>(it->second);
        }

        break;
    }
    case LUA_TNIL:
        m_value.emplace<Nil>();
        break;
    case LUA_TNONE:
    case LUA_TTHREAD:   // TODO - probably not possible
    case LUA_TFUNCTION: // TODO - has to work inside tables too
    default:
        m_value.emplace<None>();
        break;
    }
}

void CValue::Write(lua_State* L, RefList& refs) const
{
    std::visit([&, this](const auto& value) {
        using T = std::decay_t<decltype(value)>;

        LUA_CHECKSTACK(L, 1);

        if constexpr (std::is_same_v<T, OwnedTable>)
        {        
            /*
            * TL;DR; Calculate the array and hash part size for Lua, this way
            * we can reduce the number of allocations.
            * 
            * Okay, this is rough estimation of the size of the array part in Lua.
            * Because of the way luaH_next works (ltable.c) at first it returns
            * elements from the array part, thus their key can only be numeric.
            * 
            * Now, read this from the Lua implementation stuff(https://www.lua.org/doc/jucs05.pdf):
            * The computed size of the array part
            * is the largest n such that at least half the slots between 1 and n are in use
            * (to avoid wasting space with sparse arrays) and there is at least one used slot
            * between n/2 + 1 and n (to avoid a size n when n/2 would do).
            *
            * And you will roughly understand what's going on here.
            *
            * If this calculation is incorrect, that isn't a problem, but we dont
            * want to overallocate by a lot, since that wastes memory.
            * 
            * 
            */      
            //Number last = 0, empty = 0; /* last must be 0, otherwise `emptyBetween` will be -1 at first iter */
            //size_t i;
            //for (size_t end = value.size(); i < end; i++)
            //{
            //    const bool stop = std::visit([&](const auto& k) mutable {
            //        /* `n` (in the docs above) is `k` here */
            //        if constexpr (std::is_same_v<std::decay_t<decltype(k)>, Number>)
            //        {
            //            if (std::trunc(k) != k) /* check is int */
            //                return true;

            //            /* if equal with last key, we're in the hash part already.
            //             * this also handles < 0 check, since is last is always >= 0 */
            //            if (k <= last)
            //                return true;

            //            empty += k - last - 1;
            //            if (empty / 2 > k)
            //                return true; /* more than half of the slots are empty */

            //            if (last < k / 2 + 1)
            //                return true; /* no used slot between k / 2 + 1 and k */

            //            last = k;

            //            return false;
            //        }
            //        return true; /* non-numeric key found */
            //    }, value[i].first.m_value);
            //    if (stop)
            //        break;
            //}

            ///* `last` is the highest key in the array part. Add 1 for safety */
            //const int narr = static_cast<int>(last + 1);
            //const int nrec = static_cast<int>(value->size() - i + 1);
            //lua_createtable(L, value->GetArrKeysCount(), value->GetRecKeysCount());
            lua_newtable(L);
            if (value.use_count() > 1)
                value->SetRef(refs.Create(true));

            for (const auto& [k, v] : *value)
            {
                k.Write(L, refs);
                v.Write(L, refs);
                lua_rawset(L, -3);
            }
        }
        else if constexpr (std::is_same_v<T, TableRef>)
        {
            refs.Get(value->GetRef());
            dassert(lua_type(L, -1) == LUA_TTABLE);
        }
        else if constexpr (std::is_same_v<T, None>)
        {
            /* Can't push none. Do nothing. Maybe do a warning here? */
        }
        else if constexpr (std::is_same_v<T, UserData>)
        {
            lua_pushuserdata(L, reinterpret_cast<void*>(value));
        }
        else
        {
            lua::Push(L, value);
        }
    }, m_value);
}

void CValue::Write(lua_State* L) const
{
    RefList refs{ L };
    return Write(L, refs);
}

CValue::CValue(NetBitStreamInterface& bitStream, ReferencedTables& tables)
{
    SLuaTypeSync type;
    bitStream.Read(&type);

    switch (type.data.ucType)
    {
    case LUA_TNUMBER:
    {
        if (bitStream.ReadBit())
        {
            if (bitStream.ReadBit())
            {
                static_assert(std::is_same_v<double, Number>);
                Number& v = m_value.emplace<Number>();
                bitStream.Read(v);
            }
            else
            {
                float v;
                bitStream.Read(v);
                m_value = static_cast<Number>(v);
            }
        }
        else
        {
            int v;
            bitStream.Read(v);
            m_value = static_cast<Number>(v);
        }
        break;
    }
    case LUA_TSTRING:
    {
        String& value = m_value.emplace<String>();
        unsigned short length;
        bitStream.ReadCompressed(length);
        if (length) /* check if empty */
        {
            if (!bitStream.CanReadNumberOfBytes(length))
                throw std::runtime_error{ "Can't read specified number of bytes" };

            value.resize(length);
            bitStream.Read(value.data(), length); /* Read data directly into `value` */
        }
        break;
    }
    case LUA_TSTRING_LONG:
    {
        // Nearly identical to above, but with `uint` length header + aligned read
        String& value = m_value.emplace<String>();
        unsigned int length;
        bitStream.ReadCompressed(length);
        if (length) /* check if empty */
        {
            if (!bitStream.CanReadNumberOfBytes(length))
                throw std::runtime_error{ "Can't read specified number of bytes" };

            bitStream.AlignReadToByteBoundary();

            value.resize(length);
            bitStream.Read(value.data(), length); /* Read data directly into `value` */
        }
        break;
    }
    case LUA_TBOOLEAN:
    {
        bitStream.ReadBit(m_value.emplace<Bool>());
        break;
    }
    case LUA_TUSERDATA:
    case LUA_TLIGHTUSERDATA:
    {
        ElementID id;
        bitStream.Read(id);
        m_value = static_cast<UserData>(id.Value());
        break;
    }
    case LUA_TTABLE:
    {
        if (unsigned int nvals; bitStream.ReadCompressed(nvals))
        {
            auto& table = m_value.emplace<OwnedTable>(std::make_shared<Table>());
            table->reserve(nvals);
            tables.emplace_back(table);

            /* number of all elements must be even because a pair is 2 values */
            /* (Old CLuaArguments used to sture tables in a flat vector) */
            dassert(nvals % 2 == 0);

            /* originally it read k and v in separate iterations, thus nval / 2 since we read both / iteration */
            for (size_t i = 0; i < nvals / 2; i++)
            {
                CValue k = { bitStream, tables };
                CValue v = { bitStream, tables };

                /* Possible if either value is a userdata that isn't available on our side. */
                /* Have to read out both values, otherwise stream read pointer will be corrupted. */
                if (!k.Holds<Nil>() && !v.Holds<Nil>())
                    table->emplace_back(std::move(k), std::move(v));
            }
        }
        else
            throw UnableToReadFromBitStreamError{"table size"};
        break;
    }
    case LUA_TTABLEREF:
    {
        uint refid;
        bitStream.ReadCompressed(refid);
        m_value.emplace<TableRef>(tables.at(refid));
        break;
    }
    case LUA_TNIL:
        m_value.emplace<Nil>();
        break;
    case LUA_TNONE:
    default:
        m_value.emplace<None>();
        break;
    }
}

void CValue::Write(NetBitStreamInterface& bitStream, size_t& nextTableRef) const
{
    std::visit([&, this](const auto& value) {
        using T = std::decay_t<decltype(value)>;

       const auto WriteType = [&bitStream](auto type) {
            SLuaTypeSync sync;
            sync.data.ucType = type;
            sync.Write(bitStream);
        };

       if constexpr (std::is_same_v<T, Number>)
       {
           WriteType(LUA_TNUMBER);
           CompressArithmetic(value, [&bitStream, this](auto compressedValue) {
               using CT = decltype(compressedValue);
               bitStream.WriteBit(std::is_floating_point_v<CT>); /* is floating point  */
               if constexpr (std::is_same_v<CT, int>)
                   bitStream.WriteCompressed(compressedValue);
               else
               {
                   bitStream.WriteBit(std::is_same_v<CT, double>); /* is double */
                   bitStream.Write(compressedValue);
               }
               });
       }
       else if constexpr (std::is_same_v<T, String>)
       {
           const auto WriteString = [&](auto length, auto type) {
               WriteType(type);
               bitStream.WriteCompressed(length);
               if (length)
               {
                   if (type == LUA_TSTRING_LONG)
                       bitStream.AlignWriteToByteBoundary();
                   bitStream.Write(value.data(), length);
               }
           };

           if (std::numeric_limits<unsigned short>::max() >= value.length()) /* length fits in ushort? */
               WriteString(static_cast<unsigned short>(value.length()), LUA_TSTRING);
           else
               WriteString(static_cast<unsigned int>(value.length()), LUA_TSTRING_LONG); /* no, use unsigned int */
       }
       else if constexpr (std::is_same_v<T, Bool>)
       {
           WriteType(LUA_TBOOLEAN);
           bitStream.WriteBit(value);
       }
       else if constexpr (std::is_same_v<T, UserData>)
       {
           ElementID id{ value };
           if (CElementIDs::GetElement(id)) /* check if exists */
           {
               WriteType(LUA_TUSERDATA);
               bitStream.Write(id);
           }
           else
           {
               /* might happen if userdata is refernced in Lua, but element is destroyed. */
               /* it also happens if the userdata isn't an element (Vectors, Timers, XML Nodes, etc..) */
               WriteType(LUA_TNIL); /* write a nil though so other side won't get out of sync */
           }
       }
       else if constexpr (std::is_same_v<T, OwnedTable>)
       {

           WriteType(LUA_TTABLE);
           bitStream.WriteCompressed(static_cast<unsigned int>(value->size() * 2)); /* write the number of all values (as per old behaviour) */
           //if (bitStream.Can(eBitStreamVersion::CValueNArr)) /* todo: add to read as well. todo: actually implement this */
               //bitStream.WriteCompressed(value->GetArrKeysCount());
           value->SetRef(nextTableRef++);

           for (const auto& [k, v] : *value)
           {
               k.Write(bitStream, nextTableRef);
               v.Write(bitStream, nextTableRef);
           }
       }
       else if constexpr (std::is_same_v<T, TableRef>)
       {
           WriteType(LUA_TTABLEREF);
           bitStream.WriteCompressed(value->GetRef()); /* write table ref */
       }
       else if constexpr (std::is_same_v<T, Nil>)
       {
           WriteType(LUA_TNIL);
       }
       else
           throw UnableToPacketizeError{};
    }, m_value);
}

CValue::CValue(json_object* jobj, ReferencedTables& tables)
{
    switch (json_object_get_type(jobj))
    {
    case json_type_double:
    case json_type_int:
    {
        m_value.emplace<Number>(json_object_get_double(jobj));
        break;
    }
    case json_type_string:
    {
        std::string_view value{
            json_object_get_string(jobj),
            static_cast<size_t>(json_object_get_string_len(jobj))
        };
        if (!IsSpecialStringInJSON(value))
        {
            m_value.emplace<String>(value);
        }
        else /* uncommon case */
        {
            /* read number after special identifiers (first 3 chars) */
            const auto ReadNumber = [value](auto& out) {
                if (auto [p, c] = std::from_chars(value.data() + 3, value.data() + value.length(), out); c != std::errc{})
                    throw std::runtime_error{ "Failed to parse number" };
            };

            switch (value[1])
            {
            case 'T': /* table ref */
            {
                size_t ref;
                ReadNumber(ref);
                try
                {
                    m_value.emplace<TableRef>(tables.at(ref));
                }
                catch (std::out_of_range)
                {
                    throw std::runtime_error{ "Invalid table reference in JSON string" }; // TODO: std::format append value
                }
                break;
            }
            case 'R': /* resource */
            {
                std::string temp(value.begin(), value.end()); // TODO: Refactor resoruce manager to accept string_view
                if (CResource* res = GetGame()->GetResourceManager()->GetResource(temp.c_str()))
                    m_value.emplace<UserData>(res->GetScriptID());
                else
                    throw std::runtime_error{ "Invalid resource in JSON string" }; // TODO: std::format append value
                break;
            }
            case 'E': /* element */
            {
                size_t id;
                const auto begin = value.data();
                ReadNumber(id);             
                if (CElementIDs::GetElement(id)) /* check is element valid */
                    m_value.emplace<UserData>(id);
                else
                    m_value.emplace<Nil>(); /* Appears sometimes when a player quits, thus we dont throw */
                break;
            }
            }
        }
        break;
    }
    case json_type_boolean:
    {
        m_value.emplace<Bool>(json_object_get_boolean(jobj) == TRUE);
        break;
    }
    case json_type_array:
    {
        auto& table = m_value.emplace<OwnedTable>(std::make_shared<Table>());

        tables.emplace_back(table);
        table->SetRef(tables.size() - 1);
        size_t len = json_object_array_length(jobj);
        table->reserve(len);
        for (size_t i = 0; i < len; i++)
        {
            CValue k{ static_cast<Number>(i + 1) }; // Lua indices start at 1
            CValue v{ json_object_array_get_idx(jobj, i), tables };
            table->emplace_back(std::move(k), std::move(v));
        }
        break;
    }
    case json_type_object:
    {
        auto& table = m_value.emplace<OwnedTable>(std::make_shared<Table>());
        tables.emplace_back(table);
        table->SetRef(tables.size() - 1);
        table->reserve(json_object_object_length(jobj));
        json_object_object_foreach(jobj, objk, objv)
        {
            CValue k{ String{objk} };
            CValue v{ objv, tables };
            table->emplace_back(std::move(k), std::move(v));
        }
        break;
    }
    case json_type_null:
    {
        m_value.emplace<Nil>();
        break;
    }
    }

}

json_object* CValue::Write(bool serialize, size_t& nextRef) const
{
    return std::visit([&, this](const auto& value) -> json_object* {
        using T = std::decay_t<decltype(value)>;

        if constexpr (std::is_same_v<T, Number>)
        {
            int ivalue;
            if (ShouldUseInt(value, &ivalue))
                return json_object_new_int(ivalue);
            else
                return json_object_new_double(value);
        }
        else if constexpr (std::is_same_v<T, String>)
        {
            const auto DoWrite = [](const auto& str) -> json_object* {
                if (str.length() <= std::numeric_limits<unsigned short>::max())
                    return json_object_new_string_len(str.c_str(), str.length());
                else
                    throw std::runtime_error{ "Couldn't convert argument list to JSON. Invalid string specified, limit is 65535 characters." };
            };
       
            if (!IsSpecialStringInJSON(value))
                return DoWrite(value);
            else
            {
                /* Prevent clash with how MTA stores elements, resources and table refs as strings.
                 * Sadly we're buried under `const` so we can't modify the original value
                 * This is a rare case scenario anyways. */
                std::string modifiableValue{ value };
                modifiableValue[2] = '~';
                return DoWrite(modifiableValue);
            }
        }
        else if constexpr (std::is_same_v<T, Bool>)
        {
            return json_object_new_boolean(value);
        }
        else if constexpr (std::is_same_v<T, UserData>)
        {
            if (CResource* res = GetGame()->GetResourceManager()->GetResourceFromScriptID(static_cast<unsigned int>(value)))
            {
                char buffer[2 * MAX_RESOURCE_NAME_LENGTH];
            #ifdef MTA_CLIENT
                std::snprintf(buffer, sizeof(buffer), "^R^%s", res->GetName());
            #else
                std::snprintf(buffer, sizeof(buffer), "^R^%s", res->GetName().c_str());
            #endif
                return json_object_new_string(buffer);
            }
            else if (CElementIDs::GetElement(value))
            {
                if (serialize) /* Elements are dynamic, so storing them is potentially unsafe */
                {
                    char buffer[128];
                    std::snprintf(buffer, sizeof(buffer), "^E^%d", static_cast<unsigned int>(value));
                    return json_object_new_string(buffer);
                }
                else
                    throw std::runtime_error{ "Couldn't convert userdata argument to JSON, elements not allowed for this function." };
            }
            else
            {
                if (serialize)
                    throw std::runtime_error{ "Couldn't convert userdata argument to JSON, only valid elements or resources can be included." };
                else
                    throw std::runtime_error{ "Couldn't convert userdata argument to JSON, only valid resources can be included for this function." };
            }
        }
        else if constexpr (std::is_same_v<T, OwnedTable>)
        {
            /* check if consequent keys are exactly 1 apart. If they're we can use a JSON array. */
            const auto invalidArrayKeyIt = std::find_if(value->begin(), value->end(), [last = Number{ 0 }](const auto& kv) mutable {
                return std::visit([&](const auto& value) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Number>)
                    {
                        if (last + 1 != value) /* there should be exactly 1 between the two indices */
                            return true;
                        last++;
                        dassert(std::trunc(value) == value); /* check is it really numeric, if not we ~~f~~ducked up something */
                        return false;
                    }
                    return true; /* non-numeric key */
                }, kv.first.m_value);
            });

            /* Set ref before written */
            value->SetRef(nextRef++);

            if (invalidArrayKeyIt == value->end())
            {
                /* amazing, we can use an array! Yay! */
                json_object* jarray = json_object_new_array();
                for (const auto& [k, v] : *value)
                {
                    if (json_object* value = v.Write(serialize, nextRef))
                        json_object_array_add(jarray, value);
                    else
                        throw std::runtime_error{ "Failed to serialize value. Possibly out of memory." };
                }
                return jarray;
            }
            else
            {
                /* must use an object :( */
                json_object* jobject = json_object_new_object();
                for (const auto& [k, v] : *value)
                {
                    char key[255]; /* TODO: Maybe? Use a bigger buffer here / use somehow allocate buffer in jsonc */
                    key[0] = 0;
                    k.ToJSONKey(key, sizeof(key));

                    if (json_object* value = v.Write(serialize, nextRef))
                        json_object_object_add(jobject, key, value);
                    else
                        throw std::runtime_error{ "Failed to serialize value. Possibly out of memory." };
                }
                return jobject;
            }
        }
        else if constexpr (std::is_same_v<T, TableRef>)
        {
            char szTableID[64];
            std::snprintf(szTableID, sizeof(szTableID), "^T^%lu", static_cast<long unsigned>(value->GetRef()));
            return json_object_new_string(szTableID);
        }
        else if constexpr (std::is_same_v<T, Nil>)
        {
            return json_object_new_int(0);
        }
        else
            throw std::runtime_error{ "Couldn't convert argument list to JSON, unsupported data type. Use Table, Nil, String, Number, Boolean, Resource or Element." };
    }, m_value);
}

/* TODO: Refactor somehow to C++ style.. */
void CValue::ToJSONKey(char* buffer, size_t buffsize) const
{
    std::visit([&, this](const auto& value) {
        using T = std::decay_t<decltype(value)>;

        /* Can't be used as it generated compiler warning incorrectly..
        const auto PrintToBuffer = [=](auto... values) {
            std::snprintf(buffer, buffsize, values...);
        };*/

        if constexpr (std::is_same_v<T, Number>)
        {
            int ivalue;
            if (ShouldUseInt(value, &ivalue))
                std::snprintf(buffer, buffsize, "%d", ivalue);
            else
                std::snprintf(buffer, buffsize, "%f", value);
        }
        else if constexpr (std::is_same_v<T, String>)
        {
            if (value.length() <= std::numeric_limits<unsigned short>::max()) /* As per old behaviour */
            {
                if (value.length() > buffsize - 1) /* 1 for null terminator */
                {
                    GetScriptDebugging()->LogWarning(
                        nullptr, "Truncating JSOM key to %u as it's too long (%u characters).", (unsigned)buffsize, (unsigned)value.length());
                }
                /* DON'T FORGET TO USE %s AS FORMAT HERE! */
                std::snprintf(buffer, buffsize, "%s", value.c_str()); /* TODO: return c_str() here, and use that without printing to buffer */
            }
            else
                throw std::invalid_argument{ "String is too long. Limit is %u characters." };
        }
        else if constexpr (std::is_same_v<T, Bool>)
        {
            std::snprintf(buffer, buffsize, "%s", value ? "true" : "false");
        }
        else if constexpr (std::is_same_v<T, UserData>)
        {
            /* fun fact: this is never converted back :D */
            if (CElementIDs::GetElement({ value }))
            {
                std::snprintf(buffer, buffsize, "#E#%d", (int)value);
            }
            else if (CResource* res = GetGame()->GetResourceManager()->GetResourceFromScriptID(value))
            {
            #ifdef MTA_CLIENT
                std::snprintf(buffer, buffsize, "#R#%s", res->GetName());
            #else
                std::snprintf(buffer, buffsize, "#R#%s", res->GetName().c_str());
            #endif // MTA_CLIENT
            }
            else
                throw std::runtime_error{"Couldn't serialize userdata. Only valid(alive) elements can be serialized."};
        }
        else if constexpr (std::is_same_v<T, Nil>)
        {
            std::snprintf(buffer, buffsize, "0");
        }
        else if constexpr (std::is_same_v<T, OwnedTable> || std::is_same_v<T, TableRef>)
        {
            throw std::runtime_error{"Cannot convert table to string (do not use tables as keys in tables if you want to send them over http/JSON)."};
        }
        else
            throw std::runtime_error{"Couldn't convert argument to string, unsupported data type. Use String, Number, Boolean or Element."};
    }, m_value);
}

/* Table's aren't compared */
bool CValue::FlatCompare(const CValue& other) const
{
    if (other.m_value.index() != m_value.index())
        return false;

    return std::visit([&, this](const auto& value) {
        using T = std::decay_t<decltype(value)>;

        const T& otherValue = std::get<T>(other.m_value);
        if constexpr (std::is_same_v<T, OwnedTable>)
        {
            /* Lua uses hash and array part to make a table.
             * Sadly we can't just use std::equal over the 2 table to compare them as
             * value order isn't guaranteed to be the same, even if 2 tables are technically equal.
             * A solution would be to use a map: <CValue, CValue>.
             * It would be very CPU intensive in case of large tables, which isn't worth it, as 70% of the time
             * the value's won't be equal.
             * (This function is only used for setElementData)
             */
            return false;
        }
        else if constexpr (std::is_same_v<T, TableRef>)
        {
            /* TableRef's refer to a previously created table, which must've been tested by now */
            return true;
        }
        else if constexpr (std::is_same_v<T, None> || std::is_same_v<T, Nil>) /* These two are always equal */
        {
            return true;
        }
        else
        {
            return otherValue == value;
        }
    }, m_value);
}
};
