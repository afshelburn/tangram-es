#pragma once

#include "scene/styleContext.h"
#include "duktape/duktape.h"
#include "scene/scene.h"
#include "log.h"

#if 0
#define DUMP() do {                             \
        duk_push_context_dump(_ctx);           \
        LOG("%s", duk_to_string(_ctx, -1));    \
        duk_pop(_ctx);                         \
    } while(0)
#define DBG(...) LOG(__VA_ARGS__)
#else
#define DUMP()
#define DBG(...)
#endif

namespace Tangram {

using JSScopeMarker = int32_t;
using JSFunctionIndex = uint32_t;

namespace Duktape {

const static char FUNC_ID[] = "\xff""\xff""fns";
const static char GLOBAL_ID[] = "\xff""\xff""glb";
const static char FEATURE_ID[] = "\xff""\xff""fet";

struct Context;

struct Value {

    duk_context* _ctx = nullptr;
    duk_idx_t _index = 0;

    Value(duk_context* ctx, duk_idx_t index) : _ctx(ctx), _index(index) {}

    Value(Value&& _other) : _ctx(_other._ctx), _index(_other._index) {
        _other._ctx = nullptr;
    }

    Value& operator=(Value&& _other) {
        this->_ctx = _other._ctx;
        this->_index = _other._index;
        _other._ctx = nullptr;
        return *this;
    }

    operator bool() const { return bool(_ctx ); }

    bool isUndefined() {
        return duk_is_undefined(_ctx, _index) != 0;
    }

    bool isNull() {
        return duk_is_null(_ctx, _index) != 0;
    }

    bool isBoolean() {
        return duk_is_boolean(_ctx, _index) != 0;
    }

    bool isNumber() {
        return duk_is_number(_ctx, _index) != 0;
    }

    bool isString() {
        return duk_is_string(_ctx, _index) != 0;
    }

    bool isArray() {
        return duk_is_array(_ctx, _index) != 0;
    }

    bool isObject() {
        return duk_is_object(_ctx, _index) != 0;
    }

    bool toBool() {
        return duk_to_boolean(_ctx, _index) != 0;
    }

    int toInt() {
        return duk_to_int(_ctx, _index);
    }

    double toDouble() {
        return duk_to_number(_ctx, _index);
    }

    std::string toString() {
        std::string result;
        size_t len;
        // NB this requires the value to be a string. Rename to getString()?
        if (const char* data = duk_get_lstring(_ctx, _index, &len)) {
            if (len > 0) { result = std::string(data, len); }
        }
        return result;
    }

    size_t getLength() {
        return duk_get_length(_ctx, _index);
    }

    Value getValueAtIndex(size_t index) {
        duk_get_prop_index(_ctx, _index, static_cast<duk_uarridx_t>(index));
        return {_ctx, duk_normalize_index(_ctx, -1)};
    }

    Value getValueForProperty(const std::string& name) {
        duk_get_prop_lstring(_ctx, _index, name.data(), name.length());
        return {_ctx, duk_normalize_index(_ctx, -1)};
    }

    void setValueAtIndex(size_t index, Value value) {
        value.ensureExistsOnStackTop();
        duk_put_prop_index(_ctx, _index, static_cast<duk_uarridx_t>(index));
    }

    void setValueForProperty(const std::string& name, Value value) {
        value.ensureExistsOnStackTop();
        duk_put_prop_lstring(_ctx, _index, name.data(), name.length());

    }

    void ensureExistsOnStackTop() {
        auto dukTopIndex = duk_get_top_index(_ctx);
        if (_index != dukTopIndex) {
            duk_require_stack_top(_ctx, dukTopIndex + 1);
            duk_dup(_ctx, _index);
        }
    }
};

struct Function {
    JSFunctionIndex idx = 0;
    void* ptr = nullptr;
    struct {
        bool feature = false;
        bool zoom = false;
        bool geom = false;
        bool global = false;
    } context;
    std::string source;
};

static Context& getContext(duk_context* _ctx) {
    duk_memory_functions funcs;
    duk_get_memory_functions(_ctx, &funcs);
    assert(funcs.udata != nullptr);
    return *reinterpret_cast<Context*>(funcs.udata);
}

struct Context {

    duk_context* _ctx = nullptr;
    const Feature* _feature = nullptr;
    void* _featurePtr = nullptr;
    void* _objectPtr = nullptr;
    void* _functionsPtr = nullptr;
    void* _globalPtr = nullptr;

    std::vector<Function> m_functions;
    std::array<int, 4> m_filterKeys {};

    ~Context() {
        duk_destroy_heap(_ctx);
        LOG("gets:%d reused:%d - %f%% / extstr:%d freed%d\n",
            _fetchCnt+_reuseCnt, _reuseCnt,
            float(_reuseCnt) / float(_fetchCnt + _reuseCnt) * 100,
            _allocCnt, _freeCnt);
    }

    Context() {

        static std::atomic<bool> initialized{false};
        if (!initialized.exchange(true)) {
            duk_extstr_set_handler(jsExtstrInternCheck, jsExtstrFree);
        }

        // Create duktape heap with default allocation functions and
        // custom fatal error handler.
        _ctx = duk_create_heap(nullptr, nullptr, nullptr,
                               this, fatalErrorHandler);

        //// Create global geometry constants
        duk_push_number(_ctx, GeometryType::points);
        duk_put_global_string(_ctx, "point");
        duk_push_number(_ctx, GeometryType::lines);
        duk_put_global_string(_ctx, "line");
        duk_push_number(_ctx, GeometryType::polygons);
        duk_put_global_string(_ctx, "polygon");

        // Set up 'fns' array.
        duk_idx_t functionsObj = duk_push_array(_ctx);
        _functionsPtr = duk_get_heapptr(_ctx, functionsObj);
        if (!_functionsPtr) {
            LOGE("'Function object not set");
            return;
        }
        if(!duk_put_global_string(_ctx, FUNC_ID)) {
            LOGE("'Function object not set 2");
            return;
        }

        // Create 'feature' object and store in global
        // Feature object
        duk_push_object(_ctx);
        _objectPtr = duk_require_heapptr(_ctx, -1);

        // Handler object
        duk_idx_t handlerObj = duk_push_object(_ctx);
        // Add 'get' property to handler
        duk_push_c_function(_ctx, jsGetProperty, 3);
        duk_put_prop_string(_ctx, handlerObj, "get");
        // Add 'has' property to handler
        duk_push_c_function(_ctx, jsHasProperty, 2);
        duk_put_prop_string(_ctx, handlerObj, "has");
        // [{get:func,has:func}]

        duk_push_proxy(_ctx, 0);
        _featurePtr = duk_get_heapptr(_ctx, -1);
        // Stash 'feature' proxy object
        if (!duk_put_global_string(_ctx, FEATURE_ID)) {
            LOGE("Initialization failed");
            duk_pop(_ctx);
        }
        DUMP();
        DBG("<<<<<<<<<");
    }

    void setGlobalValue(const std::string& name, Value value) {
        DBG(">>>>> GLOBAL >>>>>");
        //DUMP();
        value.ensureExistsOnStackTop();

        if (name == "global") {
            _globalPtr = duk_get_heapptr(_ctx, value._index);
            if (!_globalPtr) { LOGE("Global object invalid!"); }

            // Stash global object
            duk_put_global_string(_ctx, GLOBAL_ID);
        } else {
            duk_put_global_lstring(_ctx, name.data(), name.length());
        }
        DBG("<<<<< GLOBAL <<<<<");
    }

    void setCurrentFeature(const Feature* feature) {
        _feature = feature;
        _lastFeature = nullptr;
        _propertyCacheUse = 0;
        //m_stringCache.clear();
    }

    void setFilterKey(Filter::Key _key, int _val) {
        m_filterKeys[uint8_t(_key)] = _val;
    }

    Value getStackTopValue() {
        return {_ctx, duk_normalize_index(_ctx, -1)};
    }

    JSScopeMarker getScopeMarker() {
        return duk_get_top(_ctx);
    }

    void resetToScopeMarker(JSScopeMarker marker) {
        duk_set_top(_ctx, marker);
    }

    bool evaluateBooleanFunction(JSFunctionIndex index) {
        if (!evaluateFunction(index)) { return false; }
        // Evaluate the "truthiness" of the function result at the top
        //  of the stack.
        bool result = duk_to_boolean(_ctx, -1) != 0;

        // pop result
        duk_pop(_ctx);
        return result;
    }

    Value getFunctionResult(JSFunctionIndex index) {
        if (!evaluateFunction(index)) {
            return {nullptr, 0};
        }
        return getStackTopValue();
    }

    bool setFunction(JSFunctionIndex index, const std::string& source) {
        DBG(">>>> FUNCTION >>>> %d", index);

        if (m_functions.size() == index) {
            m_functions.emplace_back();
        } else {
            m_functions.resize(index+1);
        }

        if (!duk_get_global_string(_ctx, FUNC_ID)) {
            LOGE("AddFunction - functions array not initialized");
            DUMP();
            duk_pop(_ctx); // pop [undefined] sitting at stack top
            return false;
        }
        auto& function = m_functions.back();

        bool append = false;
        std::string args;
        // TODO use proper regex

        size_t hasGlobal = source.find("global");
        if (hasGlobal != std::string::npos) {
            args += "global";
            append = true;
            function.context.global = true;
        }
        size_t hasFeature = source.find("feature");
        if (hasFeature != std::string::npos) {
            if (append) { args += ","; }
            args += "feature";
            append = true;
            function.context.feature = true;
        }
        size_t hasZoom = source.find("$zoom");
        if (hasZoom != std::string::npos) {
            if (append) { args += ","; }
            args += "$zoom";
            append = true;
            function.context.zoom = true;
        }
        size_t hasGeom = source.find("$geometry");
        if (hasGeom != std::string::npos) {
            if (append) { args += ","; }
            args += "$geometry";
            function.context.geom = true;
        }

        size_t beg = source.find('(')+1;
        function.source = source;
        function.source.insert(beg, args);

        if (duk_pcompile_lstring(_ctx, DUK_COMPILE_FUNCTION,
                                 function.source.c_str(), function.source.length()) == 0) {

#if 0
            if (hasGlobal != std::string::npos) {

                if (duk_get_prop_string(_ctx, -1, "apply") == 0)

                LOG("APPLY");

                // context
                duk_push_null(_ctx);
                // args
                duk_push_array(_ctx);
                duk_push_heapptr(_ctx, _globalPtr);
                duk_put_prop_index(_ctx, -2, 0);
                DUMP();

                //const char* pe =
                    // R"(function(f, context, args) {
                    //      if(!args) args = [];
                    //      if (args.length === f.length) return f.apply(context, args);
                    //      return function partial (a) {
                    //         var args_copy = args.concat.apply(args, arguments);
                    //         return this(f, context, args_copy);
                    //       }
                    //  })(f, context, args);)";

                //if (duk_peval_string(_ctx, pe)) {


                    if (duk_pcall(_ctx, 2) != DUK_EXEC_SUCCESS ) {
                        LOGW("Compile partial failed: %s\n%s\n---",
                             duk_safe_to_string(_ctx, -1), function.source.c_str());

                        // Pop error
                        // duk_pop(_ctx);
                        // // Pop array
                        // duk_pop(_ctx);

                    }
                    DUMP();
                }
#endif

            function.ptr = duk_get_heapptr(_ctx, -1);

            // Store function in global.functions to make sure it will not be
            // garbage collected.
            duk_put_prop_index(_ctx, -2, index);

        } else {
            LOGW("Compile failed: %s\n%s\n---",
                 duk_safe_to_string(_ctx, -1), function.source.c_str());
            // Pop error
            duk_pop(_ctx);
            // Pop array
            duk_pop(_ctx);
            return false;
        }
        // Pop the functions array off the stack
        duk_pop(_ctx);
        DBG("<<<< FUNCTION <<<<");
        return true;
    }

    bool evaluateFunction(JSFunctionIndex index) {
        //DBG(">>>>> EVAL >>>>> %d", index);

        if (m_functions.size() <= index) {
            LOGE("Functions array not initialized. index:%d size:%d",
                 index, m_functions.size());
            return false;
        }
        if (m_functions[index].ptr == nullptr) {
            LOGE("Function not set. index:%d size:%d", index, m_functions.size());
            return false;
        }

        auto &function = m_functions[index];
        duk_push_heapptr(_ctx, function.ptr);

        int args = 0;
        if (function.context.global) {
            args++;
            duk_push_heapptr(_ctx, _globalPtr);
        }
        if (function.context.feature) {
            args++;
            duk_push_heapptr(_ctx, _featurePtr);
        }
        if (function.context.zoom) {
            args++;
            duk_push_number(_ctx, m_filterKeys[uint8_t(Filter::Key::zoom)]);
        }
        if (function.context.geom) {
            args++;
            duk_push_number(_ctx, m_filterKeys[uint8_t(Filter::Key::geometry)]);
        }

        _calling = true;

        // DUMP();
        if (duk_pcall(_ctx, args) != 0) {
            _calling = false;

            LOG("Error: %s, function:%d feature:%p\n%s", duk_safe_to_string(_ctx, -1),
                index, _feature, function.source.c_str());

            // Pop error
            duk_pop(_ctx);
            return false;
        }
        _calling = false;
        // -- why not return value here?
        //DBG("<<<<< EVAL <<<<<");
        return true;
    }

    Value newNull() {
        duk_push_null(_ctx);
        return getStackTopValue();
    }

    Value newBoolean(bool value) {
        duk_push_boolean(_ctx, static_cast<duk_bool_t>(value));
        return getStackTopValue();
    }

    Value newNumber(double value) {
        duk_push_number(_ctx, value);
        return getStackTopValue();
    }

    Value newString(const std::string& value) {
        duk_push_lstring(_ctx, value.data(), value.length());
        return getStackTopValue();
    }

    Value newArray() {
        duk_push_array(_ctx);
        return getStackTopValue();
    }

    Value newObject() {
        duk_push_object(_ctx);
        return getStackTopValue();
    }

    Value newFunction(const std::string& value) {
        if (duk_pcompile_lstring(_ctx, DUK_COMPILE_FUNCTION,
                                 value.data(), value.length()) != 0) {

            auto error = duk_safe_to_string(_ctx, -1);
            LOGW("Compile failed in global function: %s\n%s\n---", error, value.c_str());
            duk_pop(_ctx); // Pop error.
            return {nullptr, 0};
        }
        return getStackTopValue();
    }

private:

    // Cache Feature property indexes
    const Feature* _lastFeature = nullptr;

    // Map strings to duk heapPtr
    std::vector<std::pair<const std::string&, const void*>> _stringCache {};

    using prop_key = const char*;
    using prop_val = const Tangram::Value*;
    using heap_ptr = void*;


    struct cache_entry {
        prop_key key = nullptr;
        prop_val val = &NOT_A_VALUE;
        heap_ptr ptr = nullptr;
    };
    std::array<cache_entry, 16> _propertyCache {};

    uint32_t _propertyCacheUse = 0;
    int _reuseCnt = 0;
    int _fetchCnt = 0;
    int _allocCnt = 0;
    int _freeCnt = 0;

    bool _calling = false;

    const char* _lastPushed = nullptr;

    static const char* jsExtstrInternCheck(void* udata, void* str, unsigned long blen) {
        Context& context = *reinterpret_cast<Context*>(udata);
        DBG("[%p / %p] >>>>> check %.*s - %d", str, context._lastPushed, blen, str, blen);
        if (context._lastPushed == str) {
            DBG("[%p] >>>>> found %.*s - %d", str, blen, str, blen);
            context._allocCnt++;
            return context._lastPushed;
        }
        return nullptr;
    }

    static void jsExtstrFree(void* udata, const void *extdata) {
        Context& context = *reinterpret_cast<Context*>(udata);
        context._freeCnt++;
        //DBG("[%p] >>>>> free %p", &context, extdata);
    }

    static cache_entry& getProperty(Context& context) {
        // Get the requested object key
        const char* key = duk_require_string(context._ctx, 1);

        auto& cache = context._propertyCache;
        size_t use = context._propertyCacheUse;

        DBG("get prop %d %s", use, key);

        for (auto i = 0; i < use; i++) {
            DBG("entry cached");
            if (cache[i].key == key) {
                context._reuseCnt++;

                return cache[i];
            }
        }

        context._fetchCnt++;

        if (use == cache.size()) {
            LOG("overflowing cache!");
            use = context._propertyCacheUse = 0;
        }

        //if (use < context->_propertyCache.size()) {
        DBG("caching");

        auto& entry= cache[use];
        entry.key = key;
        entry.val = &(context._feature->props.get(key));
        entry.ptr = nullptr;

        ++context._propertyCacheUse;
        //}
        return entry;
    }

    // Implements Proxy handler.has(target_object, key)
    static int jsHasProperty(duk_context *_ctx) {

        auto& context = getContext(_ctx);

        if (context._feature) {
            auto& entry = getProperty(context);
            bool hasProp = !(entry.val->is<none_type>());

            //if (val->is<std::string>()) {
            //const auto& str = val->get<std::string>();
            //context->m_stringCache.push_back(str.c_str());
            //DBG("push %p %s", str.c_str(), str.c_str());
            //}
            duk_push_boolean(_ctx, static_cast<duk_bool_t>(hasProp));
            return 1;
        }
        //LOGN("Error: no context found for dukctx:%p this:%p feature:%p",
        //_ctx, context, context._feature);
        return 0;
    }

    // Implements Proxy handler.get(target_object, key)
    static int jsGetProperty(duk_context *_ctx) {

        auto& context = getContext(_ctx);

        if (!context._feature) {
            //LOGN("Error: no context found for %p / %p %p",  _ctx, context, context._feature);
            return 0;
        }
        if (context._feature == context._lastFeature) {
            duk_push_heap_stash(_ctx);

            // duk_idx_t featureIdx = duk_push_heapptr(_ctx, context._objectPtr);
            duk_dup(_ctx, 1);

            if (duk_get_prop(_ctx, -2)) {
                DBG("stashed");
                return 1;
            } else {
                duk_pop(_ctx); // pop undefined
            }
            duk_pop(_ctx); // pop stash
        }
        context._lastFeature = context._feature;

        // Get the property name (second parameter)
        auto& entry = getProperty(context);

        if (entry.val->is<std::string>()) {
            const auto& str = entry.val->get<std::string>();
            if (entry.ptr) {
                DBG("push pointer %p - %s", entry.ptr, str.c_str());
                duk_push_heapptr(_ctx, entry.ptr);
            } else {
                context._lastPushed = str.c_str();

                //duk_idx_t featureIdx = duk_push_heapptr(_ctx, context._featurePtr);
                //duk_idx_t featureIdx = duk_push_heapptr(_ctx, context._objectPtr);
                duk_push_heap_stash(_ctx);

                // TODO check if we get an interned ref!
                duk_push_lstring(_ctx, str.c_str(), str.length());
                entry.ptr = duk_get_heapptr(_ctx, -1);

                duk_dup_top(_ctx); //

                // Store
                //duk_put_prop_heapptr(_ctx, featureIdx, duk_get_heapptr(_ctx, 1));
                duk_put_prop_heapptr(_ctx, -3, duk_get_heapptr(_ctx, 1));
                //duk_pop(_ctx); // pop stash
                 DBG("push string  %p %s -> %p", str.c_str(), str.c_str(), entry.ptr);
            }
        } else if (entry.val->is<double>()) {
            duk_push_number(_ctx, entry.val->get<double>());
        } else {
            duk_push_undefined(_ctx);
        }
        return 1;
    }

    static void fatalErrorHandler(void* udata, const char* message) {
        //Context* context = reinterpret_cast<Context>(udata);

        LOGE("Fatal Error in DuktapeJavaScriptContext: %s", message);
        abort();
    }
};

} // namespace Duktape
} // namespace Tangram
