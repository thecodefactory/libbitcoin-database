/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <bitcoin/database.hpp>

using namespace boost::system;
using namespace boost::filesystem;
using namespace bc;
using namespace bc::chain;
using namespace bc::database;

#define DIRECTORY "history_database"

class history_database_directory_setup_fixture
{
public:
    history_database_directory_setup_fixture()
    {
        error_code ec;
        remove_all(DIRECTORY, ec);
        BOOST_REQUIRE(create_directories(DIRECTORY, ec));
    }

    ////~history_database_directory_setup_fixture()
    ////{
    ////    error_code ec;
    ////    remove_all(DIRECTORY, ec);
    ////}
};

BOOST_FIXTURE_TEST_SUITE(database_tests, history_database_directory_setup_fixture)

BOOST_AUTO_TEST_CASE(history_database__test)
{
    const short_hash key1 = base16_literal("a006500b7ddfd568e2b036c65a4f4d6aaa0cbd9b");
    output_point out11{ hash_literal("4129e76f363f9742bc98dd3d40c99c9066e4d53b8e10e5097bd6f7b5059d7c53"), 110 };
    const size_t out_h11 = 110;
    const uint64_t value11 = 4;
    output_point out12{ hash_literal("eefa5d23968584be9d8d064bcf99c24666e4d53b8e10e5097bd6f7b5059d7c53"), 4 };
    const size_t out_h12 = 120;
    const uint64_t value12 = 8;
    output_point out13{ hash_literal("4129e76f363f9742bc98dd3d40c99c90eefa5d23968584be9d8d064bcf99c246"), 8 };
    const size_t out_h13 = 222;
    const uint64_t value13 = 6;

    input_point spend11{ hash_literal("4742b3eac32d35961f9da9d42d495ff1d90aba96944cac3e715047256f7016d1"), 0 };
    const size_t spend_h11 = 115;
    input_point spend13{ hash_literal("3cc768bbaef30587c72c6eba8dbf6aeec4ef24172ae6fe357f2e24c2b0fa44d5"), 0 };
    const size_t spend_h13 = 320;

    const short_hash key2 = base16_literal("9c6b3bdaa612ceab88d49d4431ed58f26e69b90d");
    output_point out21{ hash_literal("80d9e7012b5b171bf78e75b52d2d149580d9e7012b5b171bf78e75b52d2d1495"), 9 };
    const size_t out_h21 = 3982;
    const uint64_t value21 = 65;
    output_point out22{ hash_literal("4742b3eac32d35961f9da9d42d495ff13cc768bbaef30587c72c6eba8dbf6aee"), 0 };
    const size_t out_h22 = 78;
    const uint64_t value22 = 9;

    input_point spend22{ hash_literal("3cc768bbaef30587c72c6eba8dbfffffc4ef24172ae6fe357f2e24c2b0fa44d5"), 0 };
    const size_t spend_h22 = 900;

    const short_hash key3 = base16_literal("3eb84f6a98478e516325b70fecf9903e1ce7528b");
    output_point out31{ hash_literal("d90aba96944cac3e715047256f7016d1d90aba96944cac3e715047256f7016d1"), 0 };
    const size_t out_h31 = 378;
    const uint64_t value31 = 34;

    const short_hash key4 = base16_literal("d60db39ca8ce4caf0f7d2b7d3111535d9543473f");
    ////output_point out42{ hash_literal("aaaaaaaaaaacac3e715047256f7016d1d90aaa96944cac3e715047256f7016d1"), 0};
    const size_t out_h41 = 74448;
    const uint64_t value41 = 990;

    store::create(DIRECTORY "/history_table");
    store::create(DIRECTORY "/history_rows");
    history_database db(DIRECTORY "/history_table", DIRECTORY "/history_rows", 1000, 50);
    BOOST_REQUIRE(db.create());
    db.store(key1, { out_h11, out11, value11 });
    db.store(key1, { out_h12, out12, value12 });
    db.store(key1, { out_h13, out13, value13 });
    db.store(key1, { spend_h11, spend11, out11.checksum() });
    db.store(key1, { spend_h13, spend13, out13.checksum() });
    db.store(key2, { out_h21, out21, value21 });
    db.store(key2, { out_h22, out22, value22 });

    auto fetch_s1 = [=](const payment_record::list& history)
    {
        BOOST_REQUIRE(history.size() == 5);

        auto entry4 = history[4];
        BOOST_REQUIRE(entry4.is_valid());
        BOOST_REQUIRE(history[4].is_output());
        BOOST_REQUIRE(history[4].point().hash() == out11.hash());
        BOOST_REQUIRE(history[4].point().index() == out11.index());
        BOOST_REQUIRE(history[4].height() == out_h11);
        BOOST_REQUIRE(history[4].data() == value11);

        BOOST_REQUIRE(history[3].is_output());
        BOOST_REQUIRE(history[3].point().hash() == out12.hash());
        BOOST_REQUIRE(history[3].point().index() == out12.index());
        BOOST_REQUIRE(history[3].height() == out_h12);
        BOOST_REQUIRE(history[3].data() == value12);

        BOOST_REQUIRE(history[2].is_output());
        BOOST_REQUIRE(history[2].point().hash() == out13.hash());
        BOOST_REQUIRE(history[2].point().index() == out13.index());
        BOOST_REQUIRE(history[2].height() == out_h13);
        BOOST_REQUIRE(history[2].data() == value13);

        BOOST_REQUIRE(history[1].is_input());
        BOOST_REQUIRE(history[1].point().hash() == spend11.hash());
        BOOST_REQUIRE(history[1].point().index() == spend11.index());
        BOOST_REQUIRE(history[1].height() == spend_h11);
        BOOST_REQUIRE(history[1].data() == out11.checksum());

        BOOST_REQUIRE(history[0].is_input());
        BOOST_REQUIRE(history[0].point().hash() == spend13.hash());
        BOOST_REQUIRE(history[0].point().index() == spend13.index());
        BOOST_REQUIRE(history[0].height() == spend_h13);
        BOOST_REQUIRE(history[0].data() == out13.checksum());
    };
    auto res_s1 = db.get(key1, 0, 0);
    fetch_s1(res_s1);
    auto no_spend = [=](const payment_record::list& history)
    {
        BOOST_REQUIRE(history.size() == 2);
        BOOST_REQUIRE(history[0].is_output());
        BOOST_REQUIRE(history[1].is_output());
    };
    auto res_ns = db.get(key2, 0, 0);
    no_spend(res_ns);
    db.store(key2, { spend_h22, spend22, out22.checksum() });
    auto has_spend = [=](const payment_record::list& history)
    {
        BOOST_REQUIRE(history.size() == 3);

        BOOST_REQUIRE(history[0].is_input());
        BOOST_REQUIRE(history[0].point().hash() == spend22.hash());
        BOOST_REQUIRE(history[0].point().index() == spend22.index());
        BOOST_REQUIRE(history[0].height() == spend_h22);
        BOOST_REQUIRE(history[0].data() == out22.checksum());

        BOOST_REQUIRE(history[1].is_output());
        BOOST_REQUIRE(history[1].point().hash() == out22.hash());
        BOOST_REQUIRE(history[1].point().index() == out22.index());
        BOOST_REQUIRE(history[1].height() == out_h22);
        BOOST_REQUIRE(history[1].data() == value22);

        BOOST_REQUIRE(history[2].is_output());
        BOOST_REQUIRE(history[2].point().hash() == out21.hash());
        BOOST_REQUIRE(history[2].point().index() == out21.index());
        BOOST_REQUIRE(history[2].height() == out_h21);
        BOOST_REQUIRE(history[2].data() == value21);
    };
    auto res_has_sp = db.get(key2, 0, 0);
    has_spend(res_has_sp);
    db.unlink_last_row(key2);
    auto res_no_sp = db.get(key2, 0, 0);
    no_spend(res_no_sp);

    db.store(key3, { out_h31, out31, value31 });
    db.store(key4, { out_h41, out31, value41 });
    auto has_one_row = [=](const payment_record::list& history)
    {
        BOOST_REQUIRE(history.size() == 1);
    };
    auto res_1r1 = db.get(key3, 0, 0);
    has_one_row(res_1r1);
    auto res_1r2 = db.get(key4, 0, 0);
    has_one_row(res_1r2);
    auto has_no_rows = [=](const payment_record::list& history)
    {
        BOOST_REQUIRE(history.empty());
    };
    db.unlink_last_row(key3);
    auto res_1nr1 = db.get(key3, 0, 0);
    has_no_rows(res_1nr1);
    auto res_1nr2 = db.get(key4, 0, 0);
    has_one_row(res_1nr2);

    db.synchronize();
}

BOOST_AUTO_TEST_SUITE_END()

