/**
 * @file   unit_soma.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2022 TileDB, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * This file manages unit tests for soma objects
 */

#include <catch2/catch_test_macros.hpp>
#include <tiledb/tiledb>
#include <tiledbsoma/tiledbsoma>

#define VERBOSE 0

#ifndef TILEDBSOMA_SOURCE_ROOT
#define TILEDBSOMA_SOURCE_ROOT "not_defined"
#endif

static const std::string root = TILEDBSOMA_SOURCE_ROOT;
static const std::string soma_uri = root + "/test/soco/pbmc3k_processed";

using namespace tiledb;
using namespace tiledbsoma;

int soma_num_cells(MultiArrayBuffers& soma) {
    return soma.begin()->second.begin()->second->size();
}

TEST_CASE("SOMA: Open arrays") {
    if (VERBOSE) {
        LOG_CONFIG("debug");
    }

    Config config;
    // config.logging_level"] = "5";

    auto soma = SOMA::open(soma_uri, config);
    auto array_uris = soma->list_arrays();
    REQUIRE(array_uris.size() == 19);

    for (const auto& [name, uri] : array_uris) {
        (void)uri;
        auto array = soma->open_array(name);
    }
}

TEST_CASE("SOMA: Full query") {
    auto soma = SOMA::open(soma_uri);
    auto sq = soma->query();

    size_t total_cells = 0;
    while (auto results = sq->next_results()) {
        auto num_cells = soma_num_cells(*results);
        total_cells += num_cells;
    }
    REQUIRE(total_cells == 4848644);
}

TEST_CASE("SOMA: Sliced query (obs)") {
    auto soma = SOMA::open(soma_uri);
    auto sq = soma->query();
    auto ctx = soma->context();

    // Set obs query condition
    std::string obs_attr = "louvain";
    std::string obs_val = "B cells";
    auto obs_qc = QueryCondition::create(*ctx, obs_attr, obs_val, TILEDB_EQ);
    std::vector<std::string> obs_cols = {obs_attr};
    sq->set_obs_condition(obs_qc);
    sq->select_obs_attrs(obs_cols);

    size_t total_cells = 0;
    while (auto results = sq->next_results()) {
        auto num_cells = soma_num_cells(*results);
        total_cells += num_cells;
    }
    REQUIRE(total_cells == 628596);
}

TEST_CASE("SOMA: Sliced query (var)") {
    auto soma = SOMA::open(soma_uri);
    auto sq = soma->query();
    auto ctx = soma->context();

    // Set var query condition
    std::string var_attr = "n_cells";
    uint64_t var_val = 50;
    auto var_qc = QueryCondition::create<uint64_t>(
        *ctx, var_attr, var_val, TILEDB_LT);
    std::vector<std::string> var_cols = {var_attr};
    sq->set_var_condition(var_qc);
    sq->select_var_attrs(var_cols);

    size_t total_cells = 0;
    while (auto results = sq->next_results()) {
        auto num_cells = soma_num_cells(*results);
        total_cells += num_cells;
    }
    REQUIRE(total_cells == 1308448);
}

TEST_CASE("SOMA: Sliced query (select ids)") {
    auto soma = SOMA::open(soma_uri);
    auto sq = soma->query();

    std::vector<std::string> obs_ids = {
        "AAACATACAACCAC-1", "AAACATTGATCAGC-1", "TTTGCATGCCTCAC-1"};
    std::vector<std::string> var_ids = {"AAGAB", "AAR2", "ZRANB3"};
    sq->select_obs_ids(obs_ids);
    sq->select_var_ids(var_ids);

    size_t total_cells = 0;
    while (auto results = sq->next_results()) {
        auto num_cells = soma_num_cells(*results);
        total_cells += num_cells;
    }
    REQUIRE(total_cells == 9);
}
