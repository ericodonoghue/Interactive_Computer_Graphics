#pragma once

void InitializeTexture(cyTriMesh& mesh);

void GeometryPass();

void TextPass();

int InitializeObjectData(bool success, bool& retflag);

void CompilePrograms();

void CreateQuadAndInitializeQuadBuffers();

void InitializeObjectTextures();

void NewFunction();
