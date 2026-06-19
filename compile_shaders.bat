@rem Shader compilation script for cool-retro-term Windows port
@rem Copyright (C) 2024 pushingpandas
@rem Licensed under GPL v3 - see LICENSE file
@echo off
set QSB=qsb
set SHADERS=%~dp0app\shaders
set OPTS=--glsl "100 es,120,150" --hlsl 51 --msl 12 --qt6

echo Compiling dynamic shader variants...
for %%R in (0 1 2 3 4) do (
    for %%B in (0 1) do (
        for %%F in (0 1) do (
            for %%C in (0 1) do (
                echo   raster%%R_burn%%B_frame%%F_chroma%%C
                %QSB% %OPTS% -DCRT_RASTER_MODE=%%R -DCRT_BURN_IN=%%B -DCRT_DISPLAY_FRAME=%%F -DCRT_CHROMA=%%C -o %SHADERS%\terminal_dynamic_raster%%R_burn%%B_frame%%F_chroma%%C.frag.qsb %SHADERS%\terminal_dynamic.frag
            )
        )
    )
)

echo Compiling static shader variants...
for %%G in (0 1) do (
    for %%L in (0 1) do (
        for %%U in (0 1) do (
            for %%S in (0 1) do (
                echo   rgb%%G_bloom%%L_curve%%U_shine%%S
                %QSB% %OPTS% -DCRT_RGB_SHIFT=%%G -DCRT_BLOOM=%%L -DCRT_CURVATURE=%%U -DCRT_FRAME_SHININESS=%%S -o %SHADERS%\terminal_static_rgb%%G_bloom%%L_curve%%U_shine%%S.frag.qsb %SHADERS%\terminal_static.frag
            )
        )
    )
)

echo Compiling other shaders...
%QSB% %OPTS% -o %SHADERS%\burn_in.frag.qsb %SHADERS%\burn_in.frag
%QSB% %OPTS% -o %SHADERS%\terminal_frame.frag.qsb %SHADERS%\terminal_frame.frag
%QSB% %OPTS% -o %SHADERS%\burn_in.vert.qsb %SHADERS%\burn_in.vert
%QSB% %OPTS% -o %SHADERS%\terminal_dynamic.vert.qsb %SHADERS%\terminal_dynamic.vert
%QSB% %OPTS% -o %SHADERS%\terminal_static.vert.qsb %SHADERS%\terminal_static.vert
%QSB% %OPTS% -o %SHADERS%\terminal_frame.vert.qsb %SHADERS%\terminal_frame.vert
%QSB% %OPTS% -o %SHADERS%\passthrough.vert.qsb %SHADERS%\passthrough.vert

echo Done!
