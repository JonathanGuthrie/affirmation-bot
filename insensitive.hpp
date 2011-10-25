/*
 * Copyright 2010 Jonathan R. Guthrie
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if !defined(_INSENSITIVE_HPP_INCLUDED_)
#define _INSENSITIVE_HPP_INCLUDED_

/*
 * Implementation of a case-insensitive string
 */

#include <string>

struct caseInsensitiveTraits : public std::char_traits<char> {
  // return whether c1 and c2 are equal
  static bool eq(const char& c1, const char& c2) {
    return std::toupper(c1)==std::toupper(c2);
  }
  static bool lt(const char& c1, const char& c2) {
    return std::toupper(c1)< std::toupper(c2);
  }
  // compare up to n characters of s1 and s2
  static int compare(const char* s1, const char* s2, std::size_t n) {
    for (std::size_t i=0; i<n; ++i) {
      if (!eq(s1[i],s2[i])) {
	return lt(s1[i],s2[i])?-1:1;
      }
    }
    return 0;
  }
};


typedef std::basic_string<char, caseInsensitiveTraits> insensitiveString;

#endif // _INSENSITIVE_HPP_INCLUDED_
