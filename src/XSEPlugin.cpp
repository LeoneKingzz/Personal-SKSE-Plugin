#include "hook.h"

void OnDataLoaded()
{
   
}

void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		hooks::animEventHandler::Register(false, true);
		hooks::OnMeleeHitHook::install();
		break;

	case SKSE::MessagingInterface::kPostPostLoad:
		hooks::OnMeleeHitHook::GetSingleton()->init();
		break;
		
	default:

		break;
	}
}

void Init()
{
	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener("SKSE", MessageHandler);
}

void onSKSEInit()
{
	const auto papyrus = SKSE::GetPapyrusInterface();
	papyrus->Register(hooks::OnMeleeHitHook::BindPapyrusFunctions);
}

void InitializeLog()
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		util::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Plugin::NAME);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
	const auto level = spdlog::level::trace;
#else
	const auto level = spdlog::level::info;
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(level);
	log->flush_on(level);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%l] %v"s);
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface *a_skse)
{
#ifndef NDEBUG
	while (!IsDebuggerPresent())
	{
	};
#endif

	InitializeLog();

	logger::info("Loaded plugin");

	SKSE::Init(a_skse);

	onSKSEInit();

	Init();

	//hooks::OnMeleeHitHook::install_protected();


	return true;
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) constinit auto SKSEPlugin_Version = []() noexcept
{
	SKSE::PluginVersionData v;
	v.PluginName("PluginName");
	v.PluginVersion({1, 0, 0, 0});
	v.UsesAddressLibrary(true);
	v.HasNoStructUse(true);
	return v;
}();

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface *, SKSE::PluginInfo *pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}