/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "spo.h"
#include <random>
#include <limits>
#include <math.h>
#include <iostream>
#include <algorithm>
#include <execution>

static const int MAX_ITER_WITHOUT_IMPROVEMENT = 256;

static const double R = 0.95;
static const double THETA = 3.14159265358979323846 / 4;

static const double BOUND_OFFSET = 0.10;

// Vygeneruje search point uprostred prostoru
size_t generate_add_center(solver::TSolver_Setup& setup, double** search_points, size_t offset) {
    if (offset < setup.population_size) {
        double* center = search_points[offset];
        for (int j = 0; j < setup.problem_size; j++) {
            const double val = (setup.upper_bound[j] - setup.lower_bound[j]) / 2;
            center[j] = val;
        }
        return 1;
    }
    else {
        return 0;
    }
}

// Vygeneruje search points do vsech "rohu" prostoru
size_t generate_add_corners(solver::TSolver_Setup& setup, double** search_points, size_t offset) {
    size_t left = setup.population_size - offset;
    size_t corner_points = std::min<size_t>((size_t)std::pow(2l, setup.problem_size), left);
    for (int i = 0; i < corner_points; i++) {
        double* point = search_points[offset + i];
        for (int j = 0; j < setup.problem_size; j++) {
            const double val = (i & (1 << j))
                ? setup.lower_bound[j] + BOUND_OFFSET * (setup.upper_bound[j] - setup.lower_bound[j])
                : setup.upper_bound[j] - BOUND_OFFSET * (setup.upper_bound[j] - setup.lower_bound[j]);
            point[j] = val;
        }
    }
    return corner_points;
}

// Vygeneruje nahodne search points
size_t generate_add_random(solver::TSolver_Setup& setup, double** search_points, size_t offset) {
    std::mt19937 generator(23);
    for (int j = 0; j < setup.problem_size; j++) {
        std::uniform_real_distribution<double> random(setup.lower_bound[j], setup.upper_bound[j]);
        for (size_t i = offset; i < setup.population_size; ++i) {
            const double rnd = random(generator);
            search_points[i][j] = rnd;
        }
    }
    return setup.population_size - offset;
}

// Vygeneruje search points
double** generate_search_points(solver::TSolver_Setup& setup) {
    // initialize array
    double** search_points = new double* [setup.population_size];
    
    for (int i = 0; i < setup.population_size; ++i) {
        double* point = new double[setup.problem_size];
        search_points[i] = point;
    }

    size_t generated = 0;
    generated += generate_add_center(setup, search_points, generated);
    generated += generate_add_corners(setup, search_points, generated);
    generate_add_random(setup, search_points, generated);
    return search_points;
}

// Vynasobi dve matice
double** multiply(
    double** a, size_t am, size_t an,
    double** b, size_t bm, size_t bn,
    double** r) {

    // rm = am
    // rn = bn

    if (r == nullptr) {
        // vytvoreni nove matice
        r = new double* [am];
        for (int i = 0; i < am; ++i) {
            double* row = new double[bn];
            memset(row, 0, bn * sizeof(double));
            r[i] = row;
        }
    }
    else {
        // vynulovani matice s vysledkem
        for (int i = 0; i < am; ++i) {
            memset(r[i], 0, bn * sizeof(double));
        }
    }

    std::vector<size_t> indexes(am);
    std::iota(indexes.begin(), indexes.end(), 0);

    std::for_each(std::execution::par_unseq, indexes.begin(), indexes.end(), [&](const auto& i) {
        for (int j = 0; j < bn; ++j) {
            for (int k = 0; k < bm; ++k) {
                r[i][j] += a[i][k] * b[k][j];
            }
        }
     });

    return r;
}

// Vynasobi matici se sloupcovym vektorem, vysledek je opet sloupcovy vektor.
void multiply_vect(
    double** a, size_t am, size_t an,
    double* b,
    std::vector<double> &r) {

    // an = take velikost vektoru

  
    for (int i = 0; i < am; ++i) {
        double v = 0.0;
        for (int j = 0; j < an; ++j) {
            v += a[i][j] * b[j];
        }
        r[i] = v;
    }
    
}

// Vynasobi matici skalarem
void multiply(double** a, size_t am, size_t an, double number) {
    for (int i = 0; i < am; ++i) {
        for (int j = 0; j < an; ++j) {
            a[i][j] *= number;
        }
    }
}

// Uvolni matici
void free_2d_matrix(double** matrix, size_t rows) {
    for (int i = 0; i < rows; ++i) {
        delete[] matrix[i];
    }
    delete[] matrix;
}

// Vytvori matici R
double** create_matrix_R(size_t n, int i, int j) {
    i = i - 1;
    j = j - 1;

    double** r = new double* [n];
    for (int idx_i = 0; idx_i < n; ++idx_i) {
        double* row = new double[n];
        memset(row, 0, sizeof(double) * n);


        for (int idx_j = 0; idx_j < n; idx_j++) {
            if (idx_i == i && idx_j == i) {
                row[idx_j] = std::cos(THETA);
            }
            else if (idx_i == i && idx_j == j) {
                row[idx_j] = -std::sin(THETA);
            }
            else if (idx_i == j && idx_j == i) {
                row[idx_j] = std::sin(THETA);
            }
            else if (idx_i == j && idx_j == j) {
                row[idx_j] = std::cos(THETA);
            }
            else if (idx_i == idx_j) {
                row[idx_j] = 1;
            }
            else {
            //    row[idx_j] = 0;  - done by the memset
            }
        }
        r[idx_i] = row;
    }
    return r;
}

// Vytvori matici S
double** create_matrix_S(size_t n) {
    double** r = new double* [n];
    // inicializace matice jako jednotkove
    for (int i = 0; i < n; ++i) {
        double* row = new double[n];
        for (int j = 0; j < n; j++) {
            row[j] = (i == j) ? 1 : 0;
        }
        r[i] = row;
    }

    for (int i = 1; i < n; i++) {
        for (int j = i + 1; j <= n; j++) {
            double** tmp = create_matrix_R(n, i, j);
            double** mult = multiply(r, n, n, tmp, n, n, nullptr);
            free_2d_matrix(r, n);
            free_2d_matrix(tmp, n);
            r = mult;
        }
    }

    multiply(r, n, n, R);
    return r;
}

// Vytvori matici S-I
double** create_matrix_S_minus_I(size_t n, double** S) {
    double** SI = new double* [n];
    for (int i = 0; i < n; ++i) {
        SI[i] = new double[n];
        for (int j = 0; j < n; j++) {
            SI[i][j] = S[i][j] - (i == j ? 1 : 0);
        }
    }
    return SI;
}

// Spocte fitness pro jeden bod
double count_fitness(solver::TSolver_Setup& setup, const double* point) {
    // muze se realne stat, ze nenizsi hodnotu fitness bude mit bod mimo rozsah
    // a my nechceme aby tento bod "unesl" ostatni
    // timto ho vyradime z vyberu
    for (int j = 0; j < setup.problem_size; j++) {
        if (point[j] < setup.lower_bound[j] || point[j] > setup.upper_bound[j]) {
            return std::numeric_limits<double>::max();
        }
    }

    std::array<double, solver::Maximum_Objectives_Count> fitness;
    if (setup.objective(setup.data, 1, point, fitness.data()) != TRUE)
        fitness[0] = std::numeric_limits<double>::max();

    return fitness[0];
}

// Projde fitness hodnoty a urci minimum a sumu
void collect_fitness(solver::TSolver_Setup& setup, const std::vector<double> &fitness_results,
    double& min_fitness, int& min_fitness_index, double& sum_fitness) {

    min_fitness = std::numeric_limits<double>::max();
    min_fitness_index = -1;
    sum_fitness = 0;

    for (int i = 0; i < setup.population_size; ++i) {
        double fitness = fitness_results[i];
        if (fitness < min_fitness) {
            min_fitness = fitness;
            min_fitness_index = i;
        }
        if (fitness != std::numeric_limits<double>::max()) {
            sum_fitness += fitness;
        }
    }
}


// Spocte fitness pro vsechny body
void calculate_fitness_serial(solver::TSolver_Setup &setup, double **search_points, std::vector<double> &fitness_results) {
    for (int i = 0; i < setup.population_size; ++i) {
        fitness_results[i] = count_fitness(setup, search_points[i]);
    }
}

HRESULT solve_spo(solver::TSolver_Setup &setup, solver::TSolver_Progress &progress) {    
    progress = solver::Null_Solver_Progress;
    
    std::vector<size_t> indexes(setup.population_size);
    std::iota(indexes.begin(), indexes.end(), 0);


    int min_fitness_index;
    double min_sum_fitness = std::numeric_limits<double>::max();
    int min_sum_fitness_k = 0;    
    std::vector<double> fitness(setup.population_size);

    double **search_points = generate_search_points(setup);
    //calculate_fitness_serial(setup, search_points, fitness);
    std::for_each(std::execution::par_unseq, indexes.begin(), indexes.end(), [&](const auto& index) {
        const double* x_vec = search_points[index];
        fitness[index] = count_fitness(setup, x_vec);
        });

    collect_fitness(setup, fitness, progress.best_metric[0], min_fitness_index, min_sum_fitness);

    double **S = create_matrix_S(setup.problem_size);  // S
    double **SI = create_matrix_S_minus_I(setup.problem_size, S);  // S-I
    std::vector<double> SIc(setup.problem_size);  // (S-I)*c
    std::vector<double> Sx(setup.problem_size);  // S*x

    
    progress.max_progress = setup.max_generations;
    for (int k = 1; k < setup.max_generations; k++) {
        // otoceni podle centroidu c - ten s nejmensi fitness
        // x = S*x - (S-I)*c

        double *c_vec = search_points[min_fitness_index];
        multiply_vect(SI, setup.problem_size, setup.problem_size, c_vec, SIc);
        
        for (int i = 0; i < setup.population_size; ++i) {
            // center preskocime
            if (i == min_fitness_index) continue;

            double* x_vec = search_points[i];
            multiply_vect(S, setup.problem_size, setup.problem_size, x_vec, Sx);
            for (int j = 0; j < setup.problem_size; j++) {
                x_vec[j] = Sx[j] - SIc[j];
            }
        }


        std::for_each(std::execution::par_unseq, indexes.begin(), indexes.end(), [&](const auto &index) {
            // vypocet fitness presunuteho bodu
            const double* x_vec = search_points[index];
            fitness[index] = count_fitness(setup, x_vec);
           });

        // vypocet nove fitness
        double sum_fitness;
        collect_fitness(setup, fitness, progress.best_metric[0], min_fitness_index, sum_fitness);

        // kontrola zastaveni
        if (sum_fitness < min_sum_fitness) {
            min_sum_fitness_k = k;
            min_sum_fitness = sum_fitness;
        }
        else if (k - min_sum_fitness_k > MAX_ITER_WITHOUT_IMPROVEMENT) {
            break;
        }

        progress.current_progress = k;
    }

    // zapsani vysledku    
    for (int i = 0; i < setup.problem_size; i++) {
        setup.solution[i] = search_points[min_fitness_index][i];
    }

    // uklid
    free_2d_matrix(search_points, setup.population_size);
    free_2d_matrix(S, setup.problem_size);
    free_2d_matrix(SI, setup.problem_size);

    return S_OK;
}