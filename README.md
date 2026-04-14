# solver_bench

Инструмент для бенчмаркинга sparse direct solvers (MUMPS, SuperLU, Intel MKL PARDISO) на разреженных несимметричных матрицах формата `.mtx`.

Для каждой комбинации (матрица × солвер × число потоков) собираются: время факторизации, время решения, относительная невязка, пиковая память, оценка числа обусловленности. Результат — CSV-файл.

---

## Архитектура

Бинарный файл `solver_bench` работает в двух режимах:

- **Оркестратор** — внешний цикл: читает матрицы, запускает каждую комбинацию (солвер × потоки) как отдельный дочерний процесс через `/usr/bin/time -v`, ждёт завершения с таймаутом, пишет строку в CSV.
- **Воркер** (`--worker`) — внутренний режим: выполняет один запуск солвера, пишет результаты в stdout.

Такой подход гарантирует чистую инициализацию OMP-пула для каждого запуска и надёжный таймаут через `SIGKILL`.

---

## Требования

- CMake 3.16+
- C++17 компилятор (GCC 9+ / Clang 10+)
- gfortran
- LAPACK/BLAS
- Хотя бы один из солверов: MUMPS, SuperLU, Intel MKL PARDISO
- `time` (`apt install time` — GNU time, не shell built-in)

---

## Установка зависимостей (Ubuntu / WSL2)

```bash
sudo bash scripts/install_deps.sh
```

Скрипт устанавливает:
- `build-essential`, `cmake`, `git`, `gfortran`
- `libopenmpi-dev`, `liblapack-dev`, `libscalapack-mpi-dev`
- `libmumps-dev`, `libsuperlu-dev`
- Intel MKL (PARDISO) — только на x86_64 (пропускается на RISC-V)

На RISC-V Intel PARDISO недоступен — собирайте без `-DWITH_PARDISO=ON`.

---

## Сборка

```bash
mkdir build && cd build

# Минимум — один солвер:
cmake .. -DCMAKE_BUILD_TYPE=Release -DWITH_MUMPS=ON -DWITH_SUPERLU=ON

# Все три (только x86_64):
cmake .. -DCMAKE_BUILD_TYPE=Release -DWITH_MUMPS=ON -DWITH_SUPERLU=ON -DWITH_PARDISO=ON

make -j$(nproc)
```

Опции CMake:

| Флаг | По умолчанию | Описание |
|---|---|---|
| `WITH_MUMPS` | OFF | Включить MUMPS |
| `WITH_SUPERLU` | OFF | Включить SuperLU |
| `WITH_PARDISO` | OFF | Включить Intel MKL PARDISO |

---

## Запуск

```bash
./solver_bench \
  --matrices ../matrices/          # директория с .mtx или один файл
  --solvers mumps,superlu          # список через запятую
  --threads 1,2,4,8                # список чисел потоков
  --output ../results/bench.csv    # путь к выходному CSV
  --timeout 300                    # таймаут на один запуск, секунды (0 = без таймаута)
```

**Примеры:**

```bash
# Один солвер, один поток
./solver_bench --matrices ../matrices/orsirr_1.mtx --solvers mumps --threads 1

# Все солверы, перебор потоков
./solver_bench \
  --matrices ../matrices/ \
  --solvers mumps,superlu,pardiso \
  --threads 1,2,4,8 \
  --output ../results/bench.csv
```

---

## Формат вывода

```
matrix_name,rows,nnz,density,cond_est,solver,status,time_factorize_sec,time_solve_sec,rel_residual,memory_mb,iterations,threads
orsirr_1,1030,6858,6.457e-03,,,MUMPS,OK,3.800e-02,7.000e-03,2.300e-14,45,,1
```

| Поле | Описание |
|---|---|
| `matrix_name` | Имя файла без расширения |
| `rows` | Число строк (= столбцов) |
| `nnz` | Число ненулевых элементов |
| `density` | `nnz / (rows × cols)` |
| `cond_est` | Оценка числа обусловленности (пусто если n > 4000 или ошибка) |
| `solver` | `mumps` / `superlu` / `pardiso` |
| `status` | `OK` / `FAIL` / `TIMEOUT` |
| `time_factorize_sec` | Время факторизации |
| `time_solve_sec` | Время решения (для SuperLU = 0, т.к. API объединяет фазы) |
| `rel_residual` | `‖Ax − b‖ / ‖b‖` |
| `memory_mb` | Пиковый RSS процесса (МБ) |
| `iterations` | Всегда пусто (прямые солверы) |
| `threads` | `OMP_NUM_THREADS` при запуске |

Пустое поле = недоступное значение (pandas читает как `NaN`).

---

## Тесты

```bash
# из корня проекта:
./build/tests
```

Все тесты независимы от установленных солверов (проверяют только: MTX reader, генерацию RHS, невязку, оценку числа обусловленности, CSV writer).

---

## Тестовые матрицы

В директории `matrices/` лежат небольшие матрицы из [SuiteSparse Matrix Collection](https://sparse.tamu.edu/) для быстрой проверки:

| Матрица | n | nnz | Описание |
|---|---|---|---|
| `orsirr_1` | 1030 | 6858 | Oil reservoir simulation |
| `orsirr_2` | 886 | 5970 | Oil reservoir simulation |
| `sherman1` | 1000 | 3750 | Oil reservoir, 3-D |
| `fs_183_1` | 183 | 1069 | Fluid structure |

Для скачивания большего набора используйте [sparse.tamu.edu](https://sparse.tamu.edu/).

---

## Структура проекта

```
SolversProject/
├── CMakeLists.txt
├── src/
│   ├── main.cpp               # точка входа, диспетчер worker/orchestrator
│   ├── sparse_matrix.hpp      # CSC struct
│   ├── matrix_io.cpp/.hpp     # MTX reader → CSC
│   ├── rhs_gen.cpp/.hpp       # генерация x_true и b = A*x_true
│   ├── cond_est.cpp/.hpp      # оценка числа обусловленности (LAPACK dgecon)
│   ├── metrics.cpp/.hpp       # rel_residual
│   ├── csv_writer.cpp/.hpp    # потоковая запись CSV
│   ├── cli.cpp/.hpp           # парсинг аргументов
│   ├── solver_base.hpp        # SolveResult, SolverBase
│   ├── worker.cpp/.hpp        # режим --worker
│   ├── orchestrator.cpp/.hpp  # оркестратор
│   ├── mumps_solver.cpp/.hpp  # MUMPS реализация
│   ├── superlu_solver.cpp/.hpp # SuperLU реализация
│   └── pardiso_solver.cpp/.hpp # PARDISO реализация
├── tests/                     # Catch2 тесты
├── matrices/                  # входные .mtx файлы
├── results/                   # выходные CSV
└── scripts/
    └── install_deps.sh        # установка зависимостей Ubuntu/RISC-V
```

---

## Заметки по платформам

**WSL2 (Ubuntu, x86_64)** — полная поддержка всех трёх солверов.

**RISC-V** — MUMPS и SuperLU доступны через `apt`. Intel MKL PARDISO официально не поддерживается на RISC-V. `install_deps.sh` автоматически пропускает установку MKL на этой архитектуре.

**SuperLU** — `dgssvx` не разделяет фазы факторизации и решения: `time_factorize_sec` содержит суммарное время, `time_solve_sec = 0`.

**Число обусловленности** — вычисляется через LAPACK `dgecon` (dense LU) только для матриц n ≤ 4000. Для больших матриц поле остаётся пустым.
