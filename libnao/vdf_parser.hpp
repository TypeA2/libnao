//MIT License
//
//Copyright(c) 2016 Matthias Moeller
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files(the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions :
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#ifndef __TYTI_STEAM_VDF_PARSER_H__
#define __TYTI_STEAM_VDF_PARSER_H__

#include <unordered_map>
#include <utility>
#include <fstream>
#include <memory>

#include <system_error>
#include <exception>

//for wstring support
#include <locale>
#include <codecvt>
#include <string>

// internal
#include <stack>


//VS < 2015 has only partial C++11 support
#if defined(_MSC_VER) && _MSC_VER < 1900
#ifndef CONSTEXPR
#define CONSTEXPR
#endif

#ifndef NOEXCEPT
#define NOEXCEPT
#endif
#else
#ifndef CONSTEXPR
#define CONSTEXPR constexpr
#define TYTI_UNDEF_CONSTEXPR
#endif

#ifndef NOEXCEPT
#define NOEXCEPT noexcept
#define TYTI_UNDEF_NOEXCEPT
#endif 

#endif

namespace tyti
{
    namespace vdf
    {
        namespace detail
        {
            ///////////////////////////////////////////////////////////////////////////
            //  Helper functions selecting the right encoding (char/wchar_T)
            ///////////////////////////////////////////////////////////////////////////

            template<typename T>
            struct literal_macro_help
            {
                static CONSTEXPR const char* result(const char* c, const wchar_t* wc) NOEXCEPT
                {
                    return c;
                }
                static CONSTEXPR const char result(const char c, const wchar_t wc) NOEXCEPT
                {
                    return c;
                }
            };

            template<>
            struct literal_macro_help<wchar_t>
            {
                static CONSTEXPR const wchar_t* result(const char* c, const wchar_t* wc) NOEXCEPT
                {
                    return wc;
                }
                static CONSTEXPR const wchar_t result(const char c, const wchar_t wc) NOEXCEPT
                {
                    return wc;
                }
            };
#define TYTI_L(type, text) vdf::detail::literal_macro_help<type>::result(text, L##text)


            inline std::string string_converter(const std::string& w) NOEXCEPT
            {
                return w;
            }

            inline std::string string_converter(const std::wstring& w)
            {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> conv1; // maybe wrong econding
                return conv1.to_bytes(w);
            }

            ///////////////////////////////////////////////////////////////////////////
            //  Writer helper functions
            ///////////////////////////////////////////////////////////////////////////

            template<typename charT>
            class tabs
            {
                const size_t t;
            public:
                explicit CONSTEXPR tabs(size_t i) NOEXCEPT : t(i) {}
                std::basic_string<charT> print() const { return std::basic_string<charT>(t, TYTI_L(charT, '\t')); }
                inline CONSTEXPR tabs operator+(size_t i) const NOEXCEPT
                {
                    return tabs(i + 1);
                }
            };

            template<typename oStreamT>
            oStreamT& operator<<(oStreamT& s, const tabs<typename oStreamT::char_type> t)
            {
                s << t.print();
                return s;
            }
        } // end namespace detail


        ///////////////////////////////////////////////////////////////////////////
        //  Interface
        ///////////////////////////////////////////////////////////////////////////

        //forward decls
        //forward decl
        template<typename OutputT, typename iStreamT >
        OutputT read(iStreamT& inStream, std::error_code& ec);


        /// custom objects and their corresponding write functions

        /// basic object node. Every object has a name and can contains attributes saved as key_value pairs or childrens
        template<typename CharT>
        struct basic_object
        {
            typedef CharT char_type;
            std::basic_string<char_type> name;
            std::unordered_map<std::basic_string<char_type>, std::basic_string<char_type> > attribs;
            std::unordered_map<std::basic_string<char_type>, std::shared_ptr< basic_object<char_type> > > childs;

            void add_attribute(std::basic_string<char_type> key, std::basic_string<char_type> value)
            {
                attribs.emplace(std::move(key), std::move(value));
            }
            void add_child(std::unique_ptr< basic_object<char_type> > child)
            {
                std::shared_ptr< basic_object<char_type> > obj{ child.release() };
                childs.emplace(obj->name, obj);
            }
            void set_name(std::basic_string<char_type> n)
            {
                name = std::move(n);
            }
        };

        template<typename CharT>
        struct basic_multikey_object
        {
            typedef CharT char_type;
            std::basic_string<char_type> name;
            std::unordered_multimap<std::basic_string<char_type>, std::basic_string<char_type> > attribs;
            std::unordered_multimap<std::basic_string<char_type>, std::shared_ptr< basic_multikey_object<char_type> > > childs;

            void add_attribute(std::basic_string<char_type> key, std::basic_string<char_type> value)
            {
                attribs.emplace(std::move(key), std::move(value));
            }
            void add_child(std::unique_ptr< basic_multikey_object<char_type> > child)
            {
                std::shared_ptr< basic_multikey_object<char_type> > obj{ child.release() };
                childs.emplace(obj->name, obj);
            }
            void set_name(std::basic_string<char_type> n)
            {
                name = std::move(n);
            }
        };

        typedef basic_object<char> object;
        typedef basic_object<wchar_t> wobject;
        typedef basic_multikey_object<char> multikey_object;
        typedef basic_multikey_object<wchar_t> wmultikey_object;

        /** \brief writes given object tree in vdf format to given stream.
        Output is prettyfied, using tabs
        */
        template<typename oStreamT, typename T>
        void write(oStreamT& s, const T& r,
            const detail::tabs<typename oStreamT::char_type> tab = detail::tabs<typename oStreamT::char_type>(0))
        {
            typedef typename oStreamT::char_type charT;
            using namespace detail;
            typedef tabs<charT> tabs;
            s << tab << TYTI_L(charT, '"') << r.name << TYTI_L(charT, "\"\n") << tab << TYTI_L(charT, "{\n");
            for (const auto& i : r.attribs)
                s << tab + 1 << TYTI_L(charT, '"') << i.first << TYTI_L(charT, "\"\t\t\"") << i.second << TYTI_L(charT, "\"\n");
            for (const auto& i : r.childs)
                if (i.second) write(s, *i.second, tab + 1);
            s << tab << TYTI_L(charT, "}\n");
        }



        /** \brief Read VDF formatted sequences defined by the range [first, last).
        If the file is mailformatted, parser will try to read it until it can.
        @param first begin iterator
        @param end end iterator
        @param ec output bool. 0 if ok, otherwise, holds an system error code

        Possible error codes:
            std::errc::protocol_error: file is mailformatted
            std::errc::not_enough_memory: not enough space
            std::errc::invalid_argument: iterators throws e.g. out of range
        */

        template<typename OutputT, typename IterT>
        OutputT read(IterT first, IterT last, std::error_code& ec) NOEXCEPT
        {
            static_assert(std::is_default_constructible<OutputT>::value,
                "Output Type must be default constructible (provide constructor without arguments)");
            static_assert(std::is_move_constructible<OutputT>::value,
                "Output Type must be move constructible");

            typedef typename IterT::value_type charT;
            ec.clear();

            OutputT root;
            OutputT* cur = &root;
            std::unique_ptr<OutputT> cur_guard = nullptr; //if cur_guard == nullptr, use root, otherwise there is an heap allocated object
            std::stack< std::unique_ptr<OutputT> > lvls;

            // function for skipping a comment block
            // iter: iterator poition to the position after a '/'
            auto skip_comments = [](IterT iter, const IterT& last) -> IterT
            {
                if (iter != last)
                {
                    if (*iter == TYTI_L(charT, '/'))
                    {
                        // line comment, skip whole line
                        iter = std::find(iter + 1, last, TYTI_L(charT, '\n'));
                    }

                    if (*iter == '*')
                    {
                        // block comment, skip until next occurance of "*\"
                        const std::basic_string<charT> search_str = TYTI_L(charT, "*/");
                        iter = std::search(iter + 1, last, std::begin(search_str), std::end(search_str));
                    }
                }
                return iter;
            };

            //read header
            // first, quoted name
            auto curIter = std::find(first, last, TYTI_L(charT, '\"'));
            if (curIter == last)
            {
                ec = std::make_error_code(std::errc::protocol_error);
                return root;
            }
            {
                // extract header
                const auto headerEnd = std::find(curIter + 1, last, TYTI_L(charT, '\"'));
                root.set_name(std::basic_string<charT>(curIter + 1, headerEnd));
                // get the object section -> {}
                curIter = std::find(headerEnd, last, TYTI_L(charT, '{'));
                ++curIter;
            }
            try
            {
                if (curIter == last)
                {
                    ec = std::make_error_code(std::errc::protocol_error);
                    return root;
                }
                while (cur != nullptr && curIter != last)
                {
                    const std::basic_string<charT> startsym = TYTI_L(charT, "\"}/");

                    //find first starting attrib/child, or ending
                    curIter = std::find_first_of(curIter, last, std::begin(startsym), std::end(startsym));
                    if (*curIter == TYTI_L(charT, '\"'))
                    {

                        // get key
                        const auto keyEnd = std::find(curIter + 1, last, TYTI_L(charT, '\"'));
                        if (keyEnd == last)
                        {
                            ec = std::make_error_code(std::errc::protocol_error);// could not find end of name
                            return root;
                        }

                        std::basic_string<charT> key(curIter + 1, keyEnd);
                        curIter = keyEnd + 1;

                        const std::basic_string<charT> ecspsym = TYTI_L(charT, "\"{/");
                        curIter = std::find_first_of(curIter, last, std::begin(ecspsym), std::end(ecspsym));

                        while (*curIter == TYTI_L(charT, '/'))
                        {
                            curIter = skip_comments(curIter + 1, last);
                            curIter = std::find_first_of(curIter, last, std::begin(ecspsym), std::end(ecspsym));
                            if (curIter == last)
                            {
                                ec = std::make_error_code(std::errc::protocol_error);// could not find 2nd part of pair
                                return root;
                            }
                        }
                        // get value
                        if (*curIter == '\"')
                        {
                            const auto valueEnd = std::find(curIter + 1, last, TYTI_L(charT, '\"'));
                            if (valueEnd == last)
                            {
                                ec = std::make_error_code(std::errc::protocol_error);//could not find end of name
                                return root;
                            }

                            auto value = std::basic_string<charT>(curIter + 1, valueEnd);
                            curIter = valueEnd + 1;

                            // process value
                            if (key != TYTI_L(charT, "#include") && key != TYTI_L(charT, "#base"))
                            {
                                cur->add_attribute(std::move(key), std::move(value));
                            }
                            else
                            {
                                std::basic_ifstream<charT> i(detail::string_converter(std::move(value)));
                                auto n = std::make_unique<OutputT>(read<OutputT>(i, ec));
                                if (ec) return root;
                                cur->add_child(std::move(n));
                            }
                        }
                        else if (*curIter == '{')
                        {
                            lvls.push(std::move(cur_guard));
                            cur_guard = std::make_unique<OutputT>();
                            cur = cur_guard.get();
                            cur->set_name(std::move(key));
                            ++curIter;
                        }
                    }
                    //end of new object
                    else if (*curIter == TYTI_L(charT, '}'))
                    {
                        if (!lvls.empty())
                        {
                            //get object before
                            std::unique_ptr<OutputT> prev{ lvls.top().release() };
                            lvls.pop();

                            // add finished obj to obj before and release it from processing
                            cur = (prev == nullptr) ? &root : prev.get();
                            cur->add_child(std::move(cur_guard));
                            cur_guard = std::move(prev);
                            
                        }
                        else
                            cur = nullptr; // stops parsing
                        ++curIter;
                    }
                    else if (*curIter == TYTI_L(charT, '/'))
                    {
                        curIter = skip_comments(curIter + 1, last);
                    }

                }
            }
            catch (std::bad_alloc&)
            {
                ec = std::make_error_code(std::errc::not_enough_memory);
            }
            catch (...)
            {
                ec = std::make_error_code(std::errc::invalid_argument);
            }

            return root;

        }

        template< typename IterT >
        inline basic_object<typename IterT::value_type> read(IterT first, IterT last, std::error_code& ec) NOEXCEPT
        {
            return read< basic_object<typename IterT::value_type> >(first, last, ec);
        }

        /** \brief Read VDF formatted sequences defined by the range [first, last).
        If the file is mailformatted, parser will try to read it until it can.
        @param first begin iterator
        @param end end iterator
        @param ok output bool. true, if parser successed, false, if parser failed
        */
        template<typename OutputT, typename IterT >
        OutputT read(IterT first, const IterT last, bool* ok) NOEXCEPT
        {
            std::error_code ec;
            auto r = read<OutputT>(first, last, ec);
            if (ok) *ok = !ec;
            return r;
        }

        template<typename IterT>
        inline basic_object<typename IterT::value_type> read(IterT first, const IterT last, bool* ok) NOEXCEPT
        {
            return read< basic_object<typename IterT::value_type> >(first, last, ok);
        }

        /** \brief Read VDF formatted sequences defined by the range [first, last).
        If the file is mailformatted, parser will try to read it until it can.
        @param first begin iterator
        @param end end iterator

        throws a "std::system_error" if a parsing error occured
        */
        template<typename OutputT, typename IterT >
        OutputT read(IterT first, const IterT last)
        {
            std::error_code ec;
            const auto r = read<OutputT>(first, last, ec);
            if (ec)
                throw std::system_error(ec);
            return r;
        }

        template<typename IterT>
        inline basic_object<typename IterT::value_type> read(IterT first, const IterT last)
        {
            return read< basic_object<typename IterT::value_type> >(first, last);
        }

        /** \brief Loads a stream (e.g. filestream) into the memory and parses the vdf formatted data.
            throws "std::bad_alloc" if file buffer could not be allocated
        */
        template<typename OutputT, typename iStreamT>
        OutputT read(iStreamT& inStream, std::error_code& ec)
        {
            // cache the file
            typedef typename iStreamT::char_type charT;
            std::basic_string<charT> str;
            inStream.seekg(0, std::ios::end);
            str.resize(static_cast<size_t>(inStream.tellg()));
            if (str.empty())
                return OutputT();

            inStream.seekg(0, std::ios::beg);
            inStream.read(&str[0], str.size());

            // parse it
            return read<OutputT>(str.begin(), str.end(), ec);
        }

        template<typename iStreamT>
        inline basic_object<typename iStreamT::char_type> read(iStreamT& inStream, std::error_code& ec)
        {
            return read<basic_object<typename iStreamT::char_type> >(inStream, ec);
        }

        /** \brief Loads a stream (e.g. filestream) into the memory and parses the vdf formatted data.
            throws "std::bad_alloc" if file buffer could not be allocated
            ok == false, if a parsing error occured
        */
        template<typename OutputT, typename iStreamT>
        OutputT read(iStreamT& inStream, bool* ok)
        {
            std::error_code ec;
            const auto r = read<OutputT>(inStream, ec);
            if (ok) *ok = !ec;
            return r;
        }

        template<typename iStreamT>
        inline basic_object<typename iStreamT::char_type> read(iStreamT& inStream, bool* ok)
        {
            return read< basic_object<typename iStreamT::char_type> >(inStream, ok);
        }        

        /** \brief Loads a stream (e.g. filestream) into the memory and parses the vdf formatted data.
            throws "std::bad_alloc" if file buffer could not be allocated
            throws "std::system_error" if a parsing error occured
        */
        template<typename OutputT, typename iStreamT>
        OutputT read(iStreamT& inStream)
        {
            std::error_code ec;
            const auto r = read<OutputT>(inStream, ec);
            if (ec) throw std::system_error(ec);
            return r;
        }

        template<typename iStreamT>
        inline basic_object<typename iStreamT::char_type> read(iStreamT& inStream)
        {
            return read<basic_object<typename iStreamT::char_type>>(inStream);
        }

    } // end namespace vdf
} // end namespace tyti
#ifndef TYTI_NO_L_UNDEF
#undef TYTI_L
#endif

#ifdef TYTI_UNDEF_CONSTEXPR
#undef CONSTEXPR
#undef TYTI_NO_L_UNDEF
#endif

#ifdef TYTI_UNDEF_NOTHROW
#undef NOTHROW
#undef TYTI_UNDEF_NOTHROW
#endif

#endif //__TYTI_STEAM_VDF_PARSER_H__
