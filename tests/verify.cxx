/* Copyright 2018-2021 Dimitrij Mijoski, Sander van Geloven
 *
 * This file is part of Nuspell.
 *
 * Nuspell is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nuspell is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Nuspell.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <hunspell/hunspell.hxx>
#include <nuspell/dictionary.hxx>
#include <nuspell/finder.hxx>
#include <nuspell/utils.hxx>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unicode/ucnv.h>

#if defined(__MINGW32__) || defined(__unix__) || defined(__unix) ||            \
    (defined(__APPLE__) && defined(__MACH__)) || defined(__HAIKU__)
#include <getopt.h>
#include <unistd.h>
#endif
#ifdef _POSIX_VERSION
#include <langinfo.h>
#include <sys/resource.h>
#include <sys/time.h>
#endif

// manually define if not supplied by the build system
#ifndef PROJECT_VERSION
#define PROJECT_VERSION "unknown.version"
#endif
#define PACKAGE_STRING "nuspell " PROJECT_VERSION

using namespace std;
using namespace nuspell;

enum Mode {
	DEFAULT_MODE /**< verification test */,
	HELP_MODE /**< printing help information */,
	VERSION_MODE /**< printing version information */,
	ERROR_MODE /**< where the arguments used caused an error */
};

struct Args_t {
	Mode mode = DEFAULT_MODE;
	string program_name = "verify";
	string dictionary;
	string encoding;
	vector<string> other_dicts;
	vector<string> files;
	string correction;;
	bool print_false = false;
	bool sugs = false;
	bool print_sug = false;

	Args_t() = default;
	Args_t(int argc, char* argv[]) { parse_args(argc, argv); }
	auto parse_args(int argc, char* argv[]) -> void;
};

auto Args_t::parse_args(int argc, char* argv[]) -> void
{
	if (argc != 0 && argv[0] && argv[0][0] != '\0')
		program_name = argv[0];
// See POSIX Utility argument syntax
// http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html
#if defined(_POSIX_VERSION) || defined(__MINGW32__)
	int c;
	// The program can run in various modes depending on the
	// command line options. mode is FSM state, this while loop is FSM.
	const char* shortopts = ":d:i:c:fsphv";
	const struct option longopts[] = {
	    {"version", 0, nullptr, 'v'},
	    {"help", 0, nullptr, 'h'},
	    {nullptr, 0, nullptr, 0},
	};
	while ((c = getopt_long(argc, argv, shortopts, longopts, nullptr)) !=
	       -1) {
		switch (c) {
		case 'd':
			if (dictionary.empty())
				dictionary = optarg;
			else
				cerr << "WARNING: Detected not yet supported "
				        "other dictionary "
				     << optarg << '\n';
			other_dicts.emplace_back(optarg);

			break;
		case 'i':
			encoding = optarg;

			break;
		case 'c':
			if (correction.empty())
				correction = optarg;
			else
				cerr << "WARNING: Ignoring additional "
				        "suggestions TSV file "
				     << optarg << '\n';

			break;
		case 'f':
			print_false = true;

			break;
		case 's':
			sugs = true;
			break;
		case 'p':
			print_sug = true;
			break;
		case 'h':
			if (mode == DEFAULT_MODE)
				mode = HELP_MODE;
			else
				mode = ERROR_MODE;

			break;
		case 'v':
			if (mode == DEFAULT_MODE)
				mode = VERSION_MODE;
			else
				mode = ERROR_MODE;

			break;
		case ':':
			cerr << "Option -" << static_cast<char>(optopt)
			     << " requires an operand\n";
			mode = ERROR_MODE;

			break;
		case '?':
			cerr << "Unrecognized option: '-"
			     << static_cast<char>(optopt) << "'\n";
			mode = ERROR_MODE;

			break;
		}
	}
	files.insert(files.end(), argv + optind, argv + argc);
#endif
}

/**
 * @brief Prints help information to standard output.
 *
 * @param program_name pass argv[0] here.
 */
auto print_help(const string& program_name) -> void
{
	auto& p = program_name;
	auto& o = cout;
	o << "Usage:\n"
	     "\n";
	o << p << " [-d dict_NAME] [-i enc] [-c tsv] [-f] [-s] [file_name]...\n";
	o << p << " -h|--help|-v|--version\n";
	o << "\n"
	     "Verification testing of Nuspell for each FILE.\n"
	     "Without FILE, check standard input.\n"
	     "\n"
	     "  -d di_CT      use di_CT dictionary. Only one dictionary is\n"
	     "                currently supported\n"
	     "  -i enc        input encoding, default is active locale\n"
	     "  -i tsv        TSV file with corrections to compare suggest\n"
	     "                (this ignores FILE or standard input)\n"
	     "  -f            print false negative and false positive words\n"
	     "  -s            also test suggestions (usable only in debugger)\n"
	     "  -p            print suggestion (only when comparing suggest)\n"
	     "  -h, --help    print this help and exit\n"
	     "  -v, --version print version number and exit\n"
	     "\n";
	o << "Example: " << p << " -d en_US /usr/share/dict/american-english\n";
	o << "\n"
	     "The input should contain one word per line. Each word is\n"
	     "checked in Nuspell and Hunspell and the results are compared.\n"
	     "After all words are processed, some statistics are printed like\n"
	     "correctness and speed of Nuspell compared to Hunspell.\n"
	     "\n"
	     "Please note, messages containing:\n"
	     "  This UTF-8 encoding can't convert to UTF-16:"
	     "are caused by Hunspell and can be ignored.\n";
}

/**
 * @brief Prints the version number to standard output.
 */
auto print_version() -> void
{
	cout << PACKAGE_STRING
	    "\n"
	    "Copyright (C) 2018-2021 Dimitrij Mijoski and Sander van Geloven\n"
	    "License LGPLv3+: GNU LGPL version 3 or later "
	    "<http://gnu.org/licenses/lgpl.html>.\n"
	    "This is free software: you are free to change and "
	    "redistribute it.\n"
	    "There is NO WARRANTY, to the extent permitted by law.\n"
	    "\n"
	    "Written by Dimitrij Mijoski and Sander van Geloven.\n";
}

auto get_peak_ram_usage() -> long
{
#ifdef _POSIX_VERSION
	rusage r;
	getrusage(RUSAGE_SELF, &r);
	return r.ru_maxrss;
#else
	return 0;
#endif
}

auto to_utf8(string_view source, string& dest, UConverter* ucnv,
             UErrorCode& uerr)
{
	dest.resize(dest.capacity());
	auto len = ucnv_toAlgorithmic(UCNV_UTF8, ucnv, dest.data(), dest.size(),
	                              source.data(), source.size(), &uerr);
	dest.resize(len);
	if (uerr == U_BUFFER_OVERFLOW_ERROR) {
		uerr = U_ZERO_ERROR;
		ucnv_toAlgorithmic(UCNV_UTF8, ucnv, dest.data(), dest.size(),
		                   source.data(), source.size(), &uerr);
	}
}

auto from_utf8(string_view source, string& dest, UConverter* ucnv,
               UErrorCode& uerr)
{
	dest.resize(dest.capacity());
	auto len =
	    ucnv_fromAlgorithmic(ucnv, UCNV_UTF8, dest.data(), dest.size(),
	                         source.data(), source.size(), &uerr);
	dest.resize(len);
	if (uerr == U_BUFFER_OVERFLOW_ERROR) {
		uerr = U_ZERO_ERROR;
		ucnv_fromAlgorithmic(ucnv, UCNV_UTF8, dest.data(), dest.size(),
		                     source.data(), source.size(), &uerr);
	}
}

auto normal_loop(const Args_t& args, const Dictionary& dic, Hunspell& hun,
                 istream& in, ostream& out)
{
	auto print_false = args.print_false;
	auto test_sugs = args.sugs;
	auto word = string();
	auto u8_buffer = string();
	auto hun_word = string();
	auto total = 0;
	auto true_pos = 0;
	auto true_neg = 0;
	auto false_pos = 0;
	auto false_neg = 0;
	auto duration_hun = chrono::high_resolution_clock::duration();
	auto duration_nu = duration_hun;
	auto in_loc = in.getloc();

	auto uerr = U_ZERO_ERROR;
	auto io_cnv = icu::LocalUConverterPointer(
	    ucnv_open(args.encoding.c_str(), &uerr));
	if (U_FAILURE(uerr))
		throw runtime_error("Invalid io encoding");
	auto hun_enc =
	    nuspell::Encoding(hun.get_dict_encoding()).value_or_default();
	auto hun_cnv =
	    icu::LocalUConverterPointer(ucnv_open(hun_enc.c_str(), &uerr));
	if (U_FAILURE(uerr))
		throw runtime_error("Invalid hun encoding");
	auto io_is_utf8 = ucnv_getType(io_cnv.getAlias()) == UCNV_UTF8;
	auto hun_is_utf8 = ucnv_getType(hun_cnv.getAlias()) == UCNV_UTF8;

	// need to take entine line here, not `in >> word`
	while (getline(in, word)) {
		auto u8_word = string_view();
		auto tick_a = chrono::high_resolution_clock::now();
		if (io_is_utf8) {
			u8_word = word;
		}
		else {
			to_utf8(word, u8_buffer, io_cnv.getAlias(), uerr);
			u8_word = u8_buffer;
		}
		auto res_nu = dic.spell(u8_word);
		auto tick_b = chrono::high_resolution_clock::now();
		if (hun_is_utf8)
			hun_word = u8_word;
		else
			from_utf8(u8_word, hun_word, hun_cnv.getAlias(), uerr);
		auto res_hun = hun.spell(hun_word);
		auto tick_c = chrono::high_resolution_clock::now();
		duration_nu += tick_b - tick_a;
		duration_hun += tick_c - tick_b;
		if (res_hun) {
			if (res_nu) {
				++true_pos;
			}
			else {
				++false_neg;
				if (print_false)
					out << "FalseNegativeWord   " << word
					    << '\n';
			}
		}
		else {
			if (res_nu) {
				++false_pos;
				if (print_false)
					out << "FalsePositiveWord   " << word
					    << '\n';
			}
			else {
				++true_neg;
			}
		}
		++total;
		if (test_sugs && !res_nu && !res_hun) {
			auto nus_sugs = vector<string>();
			auto hun_sugs = vector<string>();
			dic.suggest(word, nus_sugs);
			hun.suggest(hun_word);
		}
	}
	out << "Total Words         " << total << '\n';
	// prevent devision by zero
	if (total == 0)
		return;
	auto accuracy = (true_pos + true_neg) * 1.0 / total;
	auto precision = true_pos * 1.0 / (true_pos + false_pos);
	auto speedup = duration_hun.count() * 1.0 / duration_nu.count();
	out << "True Positives      " << true_pos << '\n';
	out << "True Negatives      " << true_neg << '\n';
	out << "False Positives     " << false_pos << '\n';
	out << "False Negatives     " << false_neg << '\n';
	out << "Accuracy            " << accuracy << '\n';
	out << "Precision           " << precision << '\n';
	out << "Duration Nuspell    " << duration_nu.count() << '\n';
	out << "Duration Hunspell   " << duration_hun.count() << '\n';
	out << "Speedup Rate        " << speedup << '\n';
}

auto suggest_loop(const Args_t& args, const Dictionary& dic, Hunspell& hun,
                 istream& in, ostream& out)
{
	using chrono::high_resolution_clock;
	using chrono::nanoseconds;

	auto print_false = args.print_false;
	auto word = string();
	auto u8_buffer = string();
	auto hun_word = string();
	auto in_loc = in.getloc();
	auto correction = string();

	// verify spelling
	auto total = 0;
	auto true_pos = 0;
	auto true_neg = 0;
	auto false_pos = 0;
	auto false_neg = 0;
	auto duration_nu_tot = nanoseconds();
	auto duration_hun_tot = nanoseconds();
	auto duration_nu_min = nanoseconds(nanoseconds::max());
	auto duration_hun_min = nanoseconds(nanoseconds::max());
	auto duration_nu_max = nanoseconds();
	auto duration_hun_max = nanoseconds();
	auto speedup_max = 0.0;

	// verify suggestions
	auto sug_total = 0;
	// correction is somewhere in suggestions
	auto sug_in_nu = 0;
	auto sug_in_hun = 0;
	auto sug_in_both = 0;
	// correction is first suggestion
	auto sug_first_nu = 0;
	auto sug_first_hun = 0;
	auto sug_first_both = 0;
	auto sug_same_first = 0;
	// compared number of suggestions
	auto sug_nu_more = 0;
	auto sug_hun_more = 0;
	auto sug_same_amount = 0;
	// no suggestions
	auto sug_nu_none = 0;
	auto sug_hun_none = 0;
	auto sug_both_none = 0;
	// maximum suggestions
	auto sug_nu_max = (size_t)0;
	auto sug_hun_max = (size_t)0;
	// number suggestion at or over maximum
	auto sug_nu_at_max = 0;
	auto sug_hun_at_max = 0;
	auto sug_both_at_max = 0;

	auto sug_duration_nu_tot = nanoseconds();
	auto sug_duration_hun_tot = nanoseconds();
	auto sug_duration_nu_min = nanoseconds(nanoseconds::max());
	auto sug_duration_hun_min = nanoseconds(nanoseconds::max());
	auto sug_duration_nu_max = nanoseconds();
	auto sug_duration_hun_max = nanoseconds();
	auto sug_speedup_max = 0.0;

	auto sug_line = string();
	auto sug_excluded = vector<string>();

	auto uerr = U_ZERO_ERROR;
	auto io_cnv = icu::LocalUConverterPointer(
	    ucnv_open(args.encoding.c_str(), &uerr));
	if (U_FAILURE(uerr))
		throw runtime_error("Invalid io encoding");
	auto hun_enc =
	    nuspell::Encoding(hun.get_dict_encoding()).value_or_default();
	auto hun_cnv =
	    icu::LocalUConverterPointer(ucnv_open(hun_enc.c_str(), &uerr));
	if (U_FAILURE(uerr))
		throw runtime_error("Invalid hun encoding");
	auto io_is_utf8 = ucnv_getType(io_cnv.getAlias()) == UCNV_UTF8;
	auto hun_is_utf8 = ucnv_getType(hun_cnv.getAlias()) == UCNV_UTF8;

	// need to take entine line here, not `in >> word`
	while (getline(in, sug_line)) {
		auto word_correction = vector<string>();
		split_on_any_of(sug_line, "\t", word_correction);
		word = word_correction[0];
		correction = word_correction[1];

		auto u8_word = string_view();
		auto tick_a = high_resolution_clock::now();
		if (io_is_utf8) {
			u8_word = word;
		}
		else {
			to_utf8(word, u8_buffer, io_cnv.getAlias(), uerr);
			u8_word = u8_buffer;
		}
		auto res_nu = dic.spell(u8_word);
		auto tick_b = high_resolution_clock::now();
		if (hun_is_utf8)
			hun_word = u8_word;
		else
			from_utf8(u8_word, hun_word, hun_cnv.getAlias(), uerr);
		auto res_hun = hun.spell(hun_word);
		auto tick_c = high_resolution_clock::now();
		auto duration_nu = tick_b - tick_a;
		auto duration_hun = tick_c - tick_b;

		duration_nu_tot += duration_nu;
		duration_hun_tot += duration_hun;
		if (duration_nu < duration_nu_min)
			duration_nu_min = duration_nu;
		if (duration_hun < duration_hun_min)
			duration_hun_min = duration_hun;
		if (duration_nu > duration_nu_max)
			duration_nu_max = duration_nu;
		if (duration_hun > duration_hun_max)
			duration_hun_max = duration_hun;

		auto speedup = duration_hun * 1.0 / duration_nu;
		if (speedup > speedup_max)
			speedup_max = speedup;

		if (res_hun) {
			if (res_nu) {
				++true_pos;
			}
			else {
				++false_neg;
				if (print_false)
					out << "FalseNegativeWord   " << word
					    << '\n';
			}
		}
		else {
			if (res_nu) {
				++false_pos;
				if (print_false)
					out << "FalsePositiveWord   " << word
					    << '\n';
			}
			else {
				++true_neg;
			}
		}
		++total;

		if (res_nu || res_hun) {
			sug_excluded.push_back(word);
			continue;
		}
		// print word and expected suggestion
		if (args.print_sug)
			out << word << '\t' << correction << '\t';

		auto sugs_nu = vector<string>();
		tick_a = high_resolution_clock::now();
		dic.suggest(word, sugs_nu);
		tick_b = high_resolution_clock::now();
		auto sugs_hun = hun.suggest(hun_word);
		tick_c = high_resolution_clock::now();
		auto sug_duration_nu = tick_b - tick_a;
		auto sug_duration_hun = tick_c - tick_b;

		sug_duration_nu_tot += sug_duration_nu;
		sug_duration_hun_tot += sug_duration_hun;
		if (sug_duration_nu < sug_duration_nu_min)
			sug_duration_nu_min = sug_duration_nu;
		if (sug_duration_hun < sug_duration_hun_min)
			sug_duration_hun_min = sug_duration_hun;
		if (sug_duration_nu > sug_duration_nu_max)
			sug_duration_nu_max = sug_duration_nu;
		if (sug_duration_hun > sug_duration_hun_max)
			sug_duration_hun_max = sug_duration_hun;

		auto sug_speedup = sug_duration_hun * 1.0 / sug_duration_nu;
		if (sug_speedup > sug_speedup_max)
			sug_speedup_max = sug_speedup;

		// print number of suggestions
		if (args.print_sug)
			out << sug_duration_nu.count() << '\t'
			    << sug_duration_hun.count() << '\n';

		// correction is somewhere in suggestions
		auto in_nu = false;
		for (auto& sug : sugs_nu) {
			if (sug == correction) {
				++sug_in_nu;
				in_nu = true;
				break;
			}
		}
		for (auto& sug : sugs_hun) {
			if (sug == correction) {
				++sug_in_hun;
				if (in_nu)
					++sug_in_both;
				break;
			}
		}

		// correction is first suggestion
		auto first_nu = false;
		if (!sugs_nu.empty() && sugs_nu[0] == correction) {
			++sug_first_nu;
			first_nu = true;
		}
		if (!sugs_hun.empty() && sugs_hun[0] == correction) {
			++sug_first_hun;
			if (first_nu)
				++sug_first_both;
		}

		// same first suggestion, regardless of desired correction
		if (!sugs_nu.empty() && !sugs_hun.empty() &&
		    sugs_nu[0] == sugs_hun[0])
			++sug_same_first;

		// compared number of suggestions
		if (sugs_nu.size() == sugs_hun.size())
			++sug_same_amount;
		if (sugs_nu.size() > sugs_hun.size())
			++sug_nu_more;
		if (sugs_hun.size() > sugs_nu.size())
			++sug_hun_more;

		// no suggestions
		if (sugs_nu.empty()) {
			++sug_nu_none;
			if (sugs_hun.empty())
				++sug_both_none;
		}
		if (sugs_hun.empty())
			++sug_hun_none;

		// maximum suggestions
		if (sugs_nu.size() > sug_nu_max)
			sug_nu_max = sugs_nu.size();
		if (sugs_hun.size() > sug_hun_max)
			sug_hun_max = sugs_hun.size();

		// number suggestion at or over maximum
		if (sugs_nu.size() >= 15) {
			++sug_nu_at_max;
			if (sugs_hun.size() >= 15)
				++sug_both_at_max;
		}
		if (sugs_hun.size() >= 15)
			++sug_hun_at_max;

		++sug_total;
	}

	// prevent division by zero
	if (total == 0) {
		cerr << "WARNING: No input was provided\n";
		return;
	}
	if (duration_nu_tot.count() == 0) {
		cerr
		    << "ERROR: Invalid duration of 0 nanoseconds for Nuspell\n";
		return;
	}
	if (duration_hun_tot.count() == 0) {
		cerr << "ERROR: Invalid duration of 0 nanoseconds for "
		        "Hunspell\n";
		return;
	}

	// counts
	auto pos_nu = true_pos + false_pos;
	auto pos_hun = true_pos + false_neg;
	auto neg_nu = true_neg + false_neg;
	auto neg_hun = true_neg + false_pos;

	// rates
	auto true_pos_rate = true_pos * 1.0 / total;
	auto true_neg_rate = true_neg * 1.0 / total;
	auto false_pos_rate = false_pos * 1.0 / total;
	auto false_neg_rate = false_neg * 1.0 / total;

	auto accuracy = (true_pos + true_neg) * 1.0 / total;
	auto precision = 0.0;
	if (true_pos + false_pos != 0)
		precision = true_pos * 1.0 / (true_pos + false_pos);
	auto speedup = duration_hun_tot * 1.0 / duration_nu_tot;

	// report
	out << "Total Words Spelling        " << total << '\n';
	out << "Positives Nuspell           " << pos_nu << '\n';
	out << "Positives Hunspell          " << pos_hun << '\n';
	out << "Negatives Nuspell           " << neg_nu << '\n';
	out << "Negatives Hunspell          " << neg_hun << '\n';
	out << "True Positives              " << true_pos << '\n';
	out << "True Negatives              " << true_neg << '\n';
	out << "False Positives             " << false_pos << '\n';
	out << "False Negatives             " << false_neg << '\n';
	out << "True Positive Rate          " << true_pos_rate << '\n';
	out << "True Negative Rate          " << true_neg_rate << '\n';
	out << "False Positive Rate         " << false_pos_rate << '\n';
	out << "False Negative Rate         " << false_neg_rate << '\n';
	out << "Total Duration Nuspell      " << duration_nu_tot.count() << '\n';
	out << "Total Duration Hunspell     " << duration_hun_tot.count() << '\n';
	out << "Minimum Duration Nuspell    " << duration_nu_min.count() << '\n';
	out << "Minimum Duration Hunspell   " << duration_hun_min.count() << '\n';
	out << "Average Duration Nuspell    " << duration_nu_tot.count() / total
	    << '\n';
	out << "Average Duration Hunspell   " << duration_hun_tot.count() / total
	    << '\n';
	out << "Maximum Duration Nuspell    " << duration_nu_max.count() << '\n';
	out << "Maximum Duration Hunspell   " << duration_hun_max.count() << '\n';
	out << "Maximum Speedup             " << speedup_max << '\n';
	out << "Accuracy                    " << accuracy << '\n';
	out << "Precision                   " << precision << '\n';
	out << "Speedup                     " << speedup << '\n';

	// prevent division by zero
	if (sug_total == 0) {
		cerr << "WARNING: No input for suggestions was provided\n";
		return;
	}
	if (sug_duration_nu_tot.count() == 0) {
		cerr << "ERROR: Invalid duration of 0 nanoseconds for Nuspell "
		        "suggestions\n";
		return;
	}
	if (sug_duration_hun_tot.count() == 0) {
		cerr << "ERROR: Invalid duration of 0 nanoseconds for Hunspell "
		        "suggestions\n";
		return;
	}

	// rates
	auto sug_in_nu_rate = sug_in_nu * 1.0 / sug_total;
	auto sug_in_hun_rate = sug_in_hun * 1.0 / sug_total;
	auto sug_first_nu_rate = sug_first_nu * 1.0 / sug_total;
	auto sug_first_hun_rate = sug_first_hun * 1.0 / sug_total;

	auto sug_speedup =
	    sug_duration_hun_tot.count() * 1.0 / sug_duration_nu_tot.count();

	// suggestion report
	out << "Total Words Suggestion                  " << sug_total << '\n';
	out << "Correction In Suggestions Nuspell       " << sug_in_nu << '\n';
	out << "Correction In Suggestions Hunspell      " << sug_in_hun << '\n';
	out << "Correction In Suggestions Both          " << sug_in_both
	    << '\n';

	out << "Correction As First Suggestion Nuspell  " << sug_first_nu
	    << '\n';
	out << "Correction As First Suggestion Hunspell " << sug_first_hun
	    << '\n';
	out << "Correction As First Suggestion Both     " << sug_first_both
	    << '\n';

	out << "Nuspell More Suggestions                " << sug_nu_more
	    << '\n';
	out << "Hunspell More Suggestions               " << sug_hun_more
	    << '\n';
	out << "Same Number Of Suggestions              " << sug_in_hun << '\n';

	out << "Nuspell No Suggestions                  " << sug_nu_none
	    << '\n';
	out << "Hunspell No Suggestions                 " << sug_hun_none
	    << '\n';
	out << "Both No Suggestions                     " << sug_both_none
	    << '\n';

	out << "Maximum Suggestions Nuspell             " << sug_nu_max << '\n';
	out << "Maximum Suggestions Hunspell            " << sug_hun_max
	    << '\n';

	out << "Rate Corr. In Suggestions Nuspell       " << sug_in_nu_rate
	    << '\n';
	out << "Rate Corr. In Suggestions Hunspell      " << sug_in_hun_rate
	    << '\n';

	out << "Rate Corr. As First Suggestion Nuspell  " << sug_first_nu_rate
	    << '\n';
	out << "Rate Corr. As First Suggestion Hunspell " << sug_first_hun_rate
	    << '\n';

	out << "Total Duration Suggestions Nuspell      "
	    << sug_duration_nu_tot.count() << '\n';
	out << "Total Duration Suggestions Hunspell     "
	    << sug_duration_hun_tot.count() << '\n';
	out << "Minimum Duration Suggestions Nuspell    "
	    << sug_duration_nu_min.count() << '\n';
	out << "Minimum Duration Suggestions Hunspell   "
	    << sug_duration_hun_min.count() << '\n';
	out << "Average Duration Suggestions Nuspell    "
	    << sug_duration_nu_tot.count() / sug_total << '\n';
	out << "Average Duration Suggestions Hunspell   "
	    << sug_duration_hun_tot.count() / sug_total << '\n';
	out << "Maximum Duration Suggestions Nuspell    "
	    << sug_duration_nu_max.count() << '\n';
	out << "Maximum Duration Suggestions Hunspell   "
	    << sug_duration_hun_max.count() << '\n';
	out << "Maximum Suggestions Speedup             " << sug_speedup_max
	    << '\n';
	out << "Suggestions Speedup                     " << sug_speedup
	    << '\n';

	if (!sug_excluded.empty()) {
		out << "The following words are correct and should not be "
		       "used:\n";
		for (auto& excl : sug_excluded)
			out << excl << '\n';
	}
}

int main(int argc, char* argv[])
{
	// May speed up I/O. After this, don't use C printf, scanf etc.
	ios_base::sync_with_stdio(false);

	auto args = Args_t(argc, argv);

	switch (args.mode) {
	case HELP_MODE:
		print_help(args.program_name);
		return 0;
	case VERSION_MODE:
		print_version();
		return 0;
	case ERROR_MODE:
		cerr << "Invalid (combination of) arguments, try '"
		     << args.program_name << " --help' for more information\n";
		return 1;
	default:
		break;
	}
	auto f = Dict_Finder_For_CLI_Tool();

	auto loc_str = setlocale(LC_CTYPE, "");
	if (!loc_str) {
		clog << "WARNING: Invalid locale string, fall back to \"C\".\n";
		loc_str = setlocale(LC_CTYPE, nullptr); // will return "C"
	}
	auto loc_str_sv = string_view(loc_str);
	if (args.encoding.empty()) {
#if _POSIX_VERSION
		auto enc_str = nl_langinfo(CODESET);
		args.encoding = enc_str;
#elif _WIN32
#endif
	}
	clog << "INFO: Locale LC_CTYPE=" << loc_str_sv
	     << ", Used encoding=" << args.encoding << '\n';
	if (args.dictionary.empty()) {
		// infer dictionary from locale
		auto idx = min(loc_str_sv.find('.'), loc_str_sv.find('@'));
		args.dictionary = loc_str_sv.substr(0, idx);
	}
	if (args.dictionary.empty()) {
		cerr << "No dictionary provided and can not infer from OS "
		        "locale\n";
	}
	auto filename = f.get_dictionary_path(args.dictionary);
	if (filename.empty()) {
		cerr << "Dictionary " << args.dictionary << " not found\n";
		return 1;
	}
	clog << "INFO: Pointed dictionary " << filename << ".{dic,aff}\n";
	auto peak_ram_a = get_peak_ram_usage();
	auto dic = Dictionary();
	try {
		dic = Dictionary::load_from_path(filename);
	}
	catch (const Dictionary_Loading_Error& e) {
		cerr << e.what() << '\n';
		return 1;
	}
	auto nuspell_ram = get_peak_ram_usage() - peak_ram_a;
	auto aff_name = filename + ".aff";
	auto dic_name = filename + ".dic";
	peak_ram_a = get_peak_ram_usage();
	Hunspell hun(aff_name.c_str(), dic_name.c_str());
	auto hunspell_ram = get_peak_ram_usage() - peak_ram_a;
	cout << "Nuspell peak RAM usage:  " << nuspell_ram << "kB\n"
	     << "Hunspell peak RAM usage: " << hunspell_ram << "kB\n";
	if (!args.correction.empty()) {
		ifstream in(args.correction);
		if (!in.is_open()) {
			cerr << "Can't open " << args.correction << '\n';
			return 1;
		}
		in.imbue(cin.getloc());
		suggest_loop(args, dic, hun, in, cout);
		return 0;
	}
	if (args.files.empty()) {
		normal_loop(args, dic, hun, cin, cout);
	}
	else {
		for (auto& file_name : args.files) {
			ifstream in(file_name);
			if (!in.is_open()) {
				cerr << "Can't open " << file_name << '\n';
				return 1;
			}
			in.imbue(cin.getloc());
			normal_loop(args, dic, hun, in, cout);
		}
	}
	return 0;
}
