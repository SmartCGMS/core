/**
 * Portal for online glucose level calculation
 * Final calculation and SVG generating part
 * https://diabetes.zcu.cz/
 *
 * Author: Marek Rasocha (mrasocha@students.zcu.cz)
 * Maintainer: Martin Ubl (ublm@students.zcu.cz)
 */

#include "PlotBackgroundMap.h"

/** General **/

const std::vector<Coordinate> Coords_a = { Coordinate(70, 0), Coordinate(70, 56), Coordinate(400, 320), Coordinate(400, 400),
                                           Coordinate(400 / 1.2, 400), Coordinate(70, 84), Coordinate(175 / 3.0, 70),
                                           Coordinate(0, 70), Coordinate(0, 0) };

const std::vector<Coordinate> Coords_b_up = { Coordinate(70, 84), Coordinate(400 / 1.2, 400), Coordinate(290, 400), Coordinate(70, 180) };

const std::vector<Coordinate> Coords_b_down = { Coordinate(130, 0), Coordinate(180 , 70), Coordinate(240, 70), Coordinate(240, 180),
                                                Coordinate(400, 180), Coordinate(400, 320),Coordinate(70,56),Coordinate(70,0) };

const std::vector<Coordinate> Coords_c_up = { Coordinate(70, 180), Coordinate(290, 400), Coordinate(70, 400) };

const std::vector<Coordinate> Coords_c_down = { Coordinate(180,0), Coordinate(180,70), Coordinate(130,0) };

const std::vector<Coordinate> Coords_d_up = { Coordinate(0, 70), Coordinate(175 / 3.0, 70), Coordinate(70,84),Coordinate(70,180),Coordinate(0,180) };

const std::vector<Coordinate> Coords_d_down =  { Coordinate(240, 70), Coordinate(400, 70), Coordinate(400,180),Coordinate(240,180) };

const std::vector<Coordinate> Coords_e_up = { Coordinate(70, 180), Coordinate(70,400), Coordinate(0,400),Coordinate(0,180) };

const std::vector<Coordinate> Coords_e_down =  { Coordinate(180,0), Coordinate(400, 0), Coordinate(400,70),Coordinate(180,70) };

/** Parkes **/

const std::vector<Coordinate> Coords_a_t1 = { Coordinate(50, 0), Coordinate(50, 30), Coordinate(170, 145), Coordinate(385, 300), Coordinate(550,450), Coordinate(550, 550),
        Coordinate(430, 550), Coordinate(280,380), Coordinate(140, 170), Coordinate(30, 50),Coordinate(0,50),Coordinate(0,0) };

const std::vector<Coordinate> Coords_b_up_t1 = { Coordinate(0, 50), Coordinate(30, 50), Coordinate(140, 170), Coordinate(280,380),Coordinate(430, 550),Coordinate(260,550),Coordinate(70,110),
        Coordinate(50,80),Coordinate(30,60),Coordinate(0,60) };

const std::vector<Coordinate> Coords_b_down_t1 = { Coordinate(120, 0), Coordinate(120,30), Coordinate(260, 130), Coordinate(550, 250),Coordinate(550, 450),
        Coordinate(385, 300),Coordinate(170, 145),Coordinate(50, 30),Coordinate(50, 0) };

const std::vector<Coordinate> Coords_c_up_t1 = { Coordinate(0, 60), Coordinate(30,60), Coordinate(50,80),Coordinate(70,110),Coordinate(260,550),Coordinate(125, 550),Coordinate(80, 215),
        Coordinate(50, 125) ,Coordinate(25, 100), Coordinate(0,100) };

const std::vector<Coordinate> Coords_c_down_t1 = { Coordinate(250, 0), Coordinate(250, 40), Coordinate(550, 150), Coordinate(550, 250), Coordinate(260, 130), Coordinate(120, 30), Coordinate(120, 0) };

const std::vector<Coordinate> Coords_d_up_t1 = { Coordinate(0, 100), Coordinate(25, 100), Coordinate(50, 125),Coordinate(80, 215),Coordinate(125, 550),Coordinate(50,550), Coordinate(35,155), Coordinate(0,150) };

const std::vector<Coordinate> Coords_d_down_t1 = { Coordinate(550, 0), Coordinate(550, 150), Coordinate(250, 40), Coordinate(250, 0) };

const std::vector<Coordinate> Coords_e_up_t1 = { Coordinate(0, 150), Coordinate(35,155), Coordinate(50,550),Coordinate(0,550) };

/** diabetes 2 **/

const std::vector<Coordinate> Coords_a_t2 = { Coordinate(50, 0), Coordinate(50, 30), Coordinate(90, 80), Coordinate(330, 230), Coordinate(550,450), Coordinate(550, 550),
        Coordinate(440, 550), Coordinate(230,330),Coordinate(30, 50),Coordinate(0,50),Coordinate(0,0) };

const std::vector<Coordinate> Coords_b_up_t2 = { Coordinate(0, 50), Coordinate(30, 50), Coordinate(230,330), Coordinate(440, 550),Coordinate(280,550),
        Coordinate(30,60),Coordinate(0,60) };

const std::vector<Coordinate> Coords_b_down_t2 = { Coordinate(90, 0), Coordinate(260, 130), Coordinate(550, 250), Coordinate(550,450),Coordinate(330, 230),
        Coordinate(90, 80),Coordinate(50, 30),Coordinate(50, 0) };

const std::vector<Coordinate> Coords_c_up_t2 = { Coordinate(0, 60), Coordinate(30,60), Coordinate(280,550),Coordinate(125, 550),Coordinate(35, 90),Coordinate(25, 80),Coordinate(0, 80) };

const std::vector<Coordinate> Coords_c_down_t2 = { Coordinate(250, 0), Coordinate(250, 40), Coordinate(410,110), Coordinate(550, 160), Coordinate(550, 250), Coordinate(260, 130), Coordinate(90, 0) };

const std::vector<Coordinate> Coords_d_up_t2 = { Coordinate(0, 80), Coordinate(25, 80), Coordinate(35, 90),Coordinate(125, 550),Coordinate(50,550),Coordinate(35,200), Coordinate(0,200) };

const std::vector<Coordinate> Coords_d_down_t2 = { Coordinate(550, 160), Coordinate(410,110), Coordinate(250, 40),Coordinate(250,0),Coordinate(550,0),Coordinate(550,160) };

const std::vector<Coordinate> Coords_e_up_t2 = { Coordinate(0, 200), Coordinate(35,200), Coordinate(50,550),Coordinate(0,550) };
