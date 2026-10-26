#include <string>

typedef std::basic_string<char, std::char_traits<char>, std::allocator<char> > String;

// FUNCTION: 0x4c4ac0 ?_Tidy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAEX_N@Z
template void String::_Tidy(bool);

// FUNCTION: 0x4c4b10 ?erase@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@II@Z
template String& String::erase(String::size_type, String::size_type);

// FUNCTION: 0x4c4c30 ?_Eos@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAEXI@Z
template void String::_Eos(String::size_type);

// FUNCTION: 0x4c4c50 ?_Grow@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAE_NI_N@Z
template bool String::_Grow(String::size_type, bool);

// FUNCTION: 0x4c4fa0 ?_Copy@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAEXI@Z
template void String::_Copy(String::size_type);

// FUNCTION: 0x4c50a0 ?_Split@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AAEXXZ
template void String::_Split();

// FUNCTION: 0x4e3c00 ?assign@?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAAV12@ABV12@II@Z
template String& String::assign(const String&, String::size_type, String::size_type);
