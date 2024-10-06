/**
 * ______________________________________________________
 *
 * @file 		circular_buffer_fast_bench.cpp
 * @project 	spectre/kol-framework/
 * @author 		mkg <hello@mkg.dev>
 * @date 		17.10.2019
 *
 * @brief
 *
 * @disclaimer
 * This file is part of SPECTRE MMORPG game server project.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *
 * @copyright		2012-2019 Mustafa K. GILOR, All rights reserved.
 *
 * ______________________________________________________
 */

// proj
#include <mad/circular_buffer.hpp>
#include <mad/circular_buffer_pow2.hpp>
#include <mad/circular_buffer_vm.hpp>
#include <mad/random_bytegen.hpp>
// google.bench
#include <benchmark/benchmark.h>
// cstd
#include <unistd.h>

template <typename T>
struct cb_fixture : public benchmark::Fixture {
    void SetUp(const ::benchmark::State &) {
        mad::random::bytegen(putb);
        mad::random::bytegen(puta);
    }

    void TearDown(const ::benchmark::State &) {}

    T buffer{ static_cast<std::size_t>(getpagesize()) };
    std::array<unsigned char, 4096 / 4> putb, getb, puta;
};

////////////////////////////////////
// Circular buffer fast (mmap based)

template <class Q>
void put(benchmark::State & st) {
    Q buffer(static_cast<std::size_t>(getpagesize()));
    std::array<unsigned char, 4096 / 4> puta, putb;

    mad::random::bytegen(putb);
    mad::random::bytegen(puta);

    for (auto _ : st) {
        benchmark::DoNotOptimize(buffer.put(putb));
        benchmark::DoNotOptimize(buffer.put(puta));
        buffer.mark_as_read(putb.size());
        buffer.mark_as_read(puta.size());
    }
}

template <class Q>
void put_overwrite(benchmark::State & st) {
    Q buffer(static_cast<std::size_t>(getpagesize()));
    std::array<unsigned char, 4096 / 4> puta, putb;

    mad::random::bytegen(putb);
    mad::random::bytegen(puta);

    for (auto _ : st) {
        benchmark::DoNotOptimize(buffer.put(putb));
        benchmark::DoNotOptimize(buffer.put(puta));
    }
}

template <class Q>
void peek(benchmark::State & st) {
    Q buffer{ static_cast<std::size_t>(getpagesize()) };
    std::array<unsigned char, 4096 / 4> putb, getb;
    buffer.put(putb);
    for (auto _ : st) {
        benchmark::DoNotOptimize(buffer.peek(getb));
    }
}

template <class Q>
void putget(benchmark::State & st) {
    Q buffer{ static_cast<std::size_t>(getpagesize()) };
    std::array<unsigned char, 4096 / 4> putb, getb;
    for (auto _ : st) {
        benchmark::DoNotOptimize(buffer.put(putb));
        benchmark::DoNotOptimize(buffer.get(getb));
    }
}

// mad
BENCHMARK_TEMPLATE(put, mad::circular_buffer);
BENCHMARK_TEMPLATE(put, mad::circular_buffer_pow2);
BENCHMARK_TEMPLATE(put, mad::circular_buffer_vm<mad::vm_cb_backend_mmap>);
BENCHMARK_TEMPLATE(put, mad::circular_buffer_vm<mad::vm_cb_backend_shm>);

BENCHMARK_TEMPLATE(put_overwrite, mad::circular_buffer);
BENCHMARK_TEMPLATE(put_overwrite, mad::circular_buffer_pow2);
BENCHMARK_TEMPLATE(put_overwrite,
                   mad::circular_buffer_vm<mad::vm_cb_backend_mmap>);
BENCHMARK_TEMPLATE(put_overwrite,
                   mad::circular_buffer_vm<mad::vm_cb_backend_shm>);

BENCHMARK_TEMPLATE(peek, mad::circular_buffer);
BENCHMARK_TEMPLATE(peek, mad::circular_buffer_pow2);
BENCHMARK_TEMPLATE(peek, mad::circular_buffer_vm<mad::vm_cb_backend_mmap>);
BENCHMARK_TEMPLATE(peek, mad::circular_buffer_vm<mad::vm_cb_backend_shm>);

BENCHMARK_TEMPLATE(putget, mad::circular_buffer);
BENCHMARK_TEMPLATE(putget, mad::circular_buffer_pow2);
BENCHMARK_TEMPLATE(putget, mad::circular_buffer_vm<mad::vm_cb_backend_mmap>);
BENCHMARK_TEMPLATE(putget, mad::circular_buffer_vm<mad::vm_cb_backend_shm>);
