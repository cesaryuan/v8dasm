#include <iostream>
#include <string>

#include <libplatform/libplatform.h>
#include <v8.h>
#include <cmath>

#pragma comment(lib, "v8_libbase.lib")
#pragma comment(lib, "v8_libplatform.lib")
#pragma comment(lib, "wee8.lib")

#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dmoguids.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")
#pragma comment(lib, "msdmo.lib")
#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "DbgHelp.lib")
// 为了解决 error LNK2001 : 无法解析的外部符号 _calloc_dbg
//#pragma comment(lib, "libucrtd.lib")
using namespace v8;

static Isolate* isolate = nullptr;

static v8::ScriptCompiler::CachedData* compileCode(const char* data)
{
	auto str = String::NewFromUtf8(isolate, data).ToLocalChecked();
	auto script = Script::Compile(isolate->GetCurrentContext(), str).ToLocalChecked();
	auto unboundScript = script->GetUnboundScript();

	return ScriptCompiler::CreateCodeCache(unboundScript);
}

static void fixBytecode(uint8_t* bytecodeBuffer, const char* code) {
	auto dummyBytecode = compileCode(code);

	// Copy version hash, source hash and flag hash from dummy bytecode to source bytecode.
	// Offsets of these value may differ in different version of V8.
	// Refer V8 src/snapshot/code-serializer.h for details.
	for (int i = 4; i < 16; i++) {
		bytecodeBuffer[i] = dummyBytecode->data[i];
	}
	delete dummyBytecode;
}

static void runBytecode(uint8_t* bytecodeBuffer, int len) {
	// Compile some dummy code to get version hash, source hash and flag hash.
	const char* code = "1111";
	fixBytecode(bytecodeBuffer, code);

	// Load code into code cache.
	auto cached_data = new ScriptCompiler::CachedData(bytecodeBuffer, len);

	// Create dummy source.
	ScriptOrigin origin(String::NewFromUtf8Literal(isolate, "code.jsc"));
	ScriptCompiler::Source source(String::NewFromUtf8(isolate, code).ToLocalChecked(), origin, cached_data);

	// Compile code from code cache to print disassembly.
	MaybeLocal<UnboundScript> v8_script =
		ScriptCompiler::CompileUnboundScript(isolate, &source, ScriptCompiler::kConsumeCodeCache);
}

static void readAllBytes(const std::string& file, std::vector<char>& buffer) {
	std::ifstream infile(file, std::ifstream::binary);

	infile.seekg(0, infile.end);
	size_t length = infile.tellg();
	infile.seekg(0, infile.beg);

	if (length > 0) {
		buffer.resize(length);
		infile.read(&buffer[0], length);
	}
}

int main1(int argc, char* argv[])
{
	// Set flags here, flags that affects code generation and seririalzation should be same as the target program.
	// You can add other flags freely because flag hash will be overrided in fixBytecode().
	v8::V8::SetFlagsFromString("--no-lazy --no-flush-bytecode --log-all --no-verify-snapshot-checksum");

	v8::V8::InitializeICU();
	auto plat = v8::platform::NewDefaultPlatform();
	v8::V8::InitializePlatform(plat.get());
	v8::V8::Initialize();

	Isolate::CreateParams p = {};
	p.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();

	isolate = Isolate::New(p);
	{
		v8::HandleScope scope(isolate);
		auto ctx = v8::Context::New(isolate);
		Context::Scope context_scope(ctx);

		std::vector<char> data;
		readAllBytes(argv[1], data);
		runBytecode((uint8_t*)data.data(), data.size());
	}
	return 0;
}

int main(int argc, char* argv[]) {
	v8::V8::SetFlagsFromString("--no-lazy --no-flush-bytecode --log-all");
    // Initialize V8.
    v8::V8::InitializeICUDefaultLocation(argv[0]);
    v8::V8::InitializeExternalStartupData(argv[0]);
    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();
    // Create a new Isolate and make it the current one.
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    isolate = v8::Isolate::New(create_params);
    {
        v8::Isolate::Scope isolate_scope(isolate);
        // Create a stack-allocated handle scope.
        v8::HandleScope handle_scope(isolate);
        // Create a new context.
        v8::Local<v8::Context> context = v8::Context::New(isolate);
        // Enter the context for compiling and running the hello world script.
        v8::Context::Scope context_scope(context);
        {
			std::vector<char> code;
			readAllBytes("example.js", code);
			code.push_back('\0');
			auto cdata = compileCode(code.data());
			runBytecode(const_cast<uint8_t*>(cdata->data), cdata->length);
        }
    }
    // Dispose the isolate and tear down V8.
    isolate->Dispose();
    v8::V8::Dispose();
    delete create_params.array_buffer_allocator;
    return 0;
}