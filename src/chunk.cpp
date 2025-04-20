#include "chunk.hpp"

//////////////////////////Create chunk noise map generator function using FastNoise/////////////
#pragma region generate_terrain {

FastNoise::SmartNode<> Generate_Terrain() {
    auto OpenSimplex = FastNoise::New<FastNoise::OpenSimplex2>();
    auto FractalFBm = FastNoise::New<FastNoise::FractalFBm>();
    FractalFBm->SetSource(OpenSimplex);
    FractalFBm->SetGain(0.06f);
    FractalFBm->SetOctaveCount(4);
    FractalFBm->SetLacunarity(4.0f);
    auto DomainScale = FastNoise::New<FastNoise::DomainScale>();
    DomainScale->SetSource(FractalFBm);
    DomainScale->SetScale(1.64f);
    auto PositionOutput = FastNoise::New<FastNoise::PositionOutput>();
    PositionOutput->Set<FastNoise::Dim::Y>(6.72f);
    auto add = FastNoise::New<FastNoise::Add>();
    add->SetLHS(DomainScale);
    add->SetRHS(PositionOutput);

    return add;
}

#pragma endregion generate_terrain }
////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////Create grass noise map generator function using FastNoise////////////
#pragma region generate_grass {

FastNoise::SmartNode<> Generate_Grass() {
    auto Perlin = FastNoise::New<FastNoise::Perlin>();
    auto add = FastNoise::New<FastNoise::Add>();
    add->SetLHS(Perlin);
    add->SetRHS(-.4);

    return add;
}

#pragma endregion generate_grass }
////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////Generate noise map for single chunk///////////////////////////////////
void generateNoise(FastNoise::SmartNode<> generator, std::vector<float>& noiseMap, int x, int y, int seed)  {
    generator->GenUniformGrid3D(noiseMap.data(), 16 * x, -20,16 * y, 16, 128, 16, 0.01f, seed);
}
////////////////////////////////////////////////////////////////////////////////////////////////

void ChunkClass::generateChunk(std::unordered_map<std::string, int> texturesDict) {
    //////////////////////////////Chunk creator//////////////////////////////////////
    std::vector<Chunk> chunk = {};
    chunk.resize(chunkCount);

    std::memset(chunk.data(), 0, sizeof(Chunk)* chunk.size());

    ////////////////////////////////////////Generate new map/////////////////////////////////////////
    bool loadMap;
    std::ifstream mapFileTest(mapFileName + ".txt");
    if(mapFileTest.good()) {
        loadMap = true;
    }
    else {
        loadMap = false;
    }
    mapFileTest.close();
    if(!loadMap || regenerate) {
        NoiseMaps noiseMaps[chunkCount];
        auto generator = Generate_Terrain();
        std::vector<glm::vec2> positions;
        

        for(int i = 0; i < chunkCount; i++) {
            noiseMaps[i].noiseMap = std::vector<float>(128*16*16);
        }

        srand(time(0));
        seed = rand() % 100000;

        if(chunkLine < 32) {
            std::vector<std::thread> threads;
            int counter = 0;
            for(int i = 0; i < chunkLine; i++) {
                for(int j = 0; j < chunkLine; j++) {
                    threads.emplace_back(std::thread(generateNoise, generator, std::ref(noiseMaps[counter].noiseMap), i - chunkLine/2, j - chunkLine/2, seed));
                    counter++;
                } 
            }

            int count = 0;
            for (auto& t : threads) {
                t.join();
                std::cout << "Generating chunks: " << round((float)count/(float)(chunkCount-1)*100) << "%" << "\t\r" << std::flush;
                count++;
            }
        }
        else {
            std::vector<std::thread> threads;
            int counter = 0;
            int count = 0;
            int posX = 0;
            int posY = 0;
            for(int k = 0; k < chunkLine/32*chunkLine/32; k++) {
                threads.clear();
                for(int i = 0; i < 32 * 32; i++) {
                    threads.emplace_back(std::thread(generateNoise, generator, std::ref(noiseMaps[counter].noiseMap), posX - chunkLine/2, posY - chunkLine/2, seed));
                    posY++;
                    counter++;
                    if(counter % chunkLine == 0) {
                        posY = 0;
                        posX++;
                    }
                }

                for (auto& t : threads) {
                    t.join();
                    std::cout << "Generating chunks: " << round((float)count/(float)(chunkCount-1)*100) << "%, " << k << "/" << chunkLine/32*chunkLine/32 << "\t\r" << std::flush;
                    count++;
                }
            }
            std::cout << "Generating chunks: " << round((float)count/(float)(chunkCount-1)*100) << "%, " << chunkLine/32*chunkLine/32 << "/" << chunkLine/32*chunkLine/32 << "\t\r" << std::flush;
        }

        int count = 0;
        for(int i = 0; i < chunkLine;i++) {
            for(int j = 0; j < chunkLine; j++) {
                glm::vec2 position = glm::vec2{i - chunkLine/2, j - chunkLine/2};
                positions.push_back(position);
                // auto generateStart = std::chrono::high_resolution_clock::now();
                // auto generateEnd = std::chrono::high_resolution_clock::now();
                // auto duration = duration_cast<std::chrono::milliseconds>(generateEnd - generateStart);
                // std::cout << "Generating chunk number: " << count << " " << duration.count() << std::endl;
                int index = 0;
                // auto generatorStart = std::chrono::high_resolution_clock::now();
                for(int x = 0; x < 16; x++) {
                    for(int y = 0; y < 128; y++) {
                        for(int z = 0; z < 16; z++) {
                            if(noiseMaps[count].noiseMap[index] >= 0.0f) {
                                chunk[count].blocks[y][x][z] = 0;
                            }
                            else {
                                if(noiseMaps[count].noiseMap[index + 16] >= 0) {
                                    chunk[count].blocks[y][x][z] = blockFace(glm::vec3 {x,y,z}, texturesDict.at("grass_block"));
                                } 
                                else if(noiseMaps[count].noiseMap[index + 80] >= 0) {
                                    chunk[count].blocks[y][x][z] = blockFace(glm::vec3 {x,y,z}, texturesDict.at("dirt_block"));
                                } 
                                else if(noiseMaps[count].noiseMap[index] < 0) {
                                    chunk[count].blocks[y][x][z] = blockFace(glm::vec3 {x,y,z}, texturesDict.at("stone_block"));
                                }
                            }
                            index++;
                        }
                    }
                }
                count++;
                // auto generatorEnd = std::chrono::high_resolution_clock::now();
                // auto duration1 = duration_cast<std::chrono::milliseconds>(generatorEnd - generatorStart);
                // std::cout << "Setting into chunks time: " << duration1.count() << std::endl;
            }
        }
        std::cout << std::endl;
        int index = 0;
        std::vector<float> grassNoiseMap(16*16*chunkCount);
        auto grass_generator = Generate_Grass();
        grass_generator->GenUniformGrid2D(grassNoiseMap.data(), 0, 0, chunkLine*16,chunkLine*16, 0.5f, seed);
        for(int i = 0; i < chunkCount;i++) {
            for(int j = 0; j < 16; j++) {
                for(int k = 0; k < 16; k++) {
                    if(grassNoiseMap[index] > 0) {
                        int counter = 0;
                        while(true) {
                            if(chunk[i].blocks[counter][j][k] == 0) {
                                chunk[i].blocks[counter][j][k] = blockFace(glm::vec3 {j,counter,k}, texturesDict.at("plantGrass_block"));
                                break;
                            }
                            counter++;
                        }
                    }
                    index++;
                }
            }
        }
        // auto generatorStart = std::chrono::high_resolution_clock::now();
        std::ofstream chunkFileWrite(mapFileName +".txt");
        chunkFileWrite << seed;
        int counter = 0;
        for(int k = 0; k < chunkCount; k++) {
            std::cout << "Saving chunks: " << round((float)k/(float)(chunkCount-1)*100) << "%" << "\t\r" << std::flush;
            chunkFileWrite << "-" << k << "~";
            chunkFileWrite << ChunkPosition(positions[k].x, positions[k].y) << " ";
            for(int i = 0; i < 128; i++) {
                for(int l = 0; l < 16; l++) {
                    for(int m = 0; m < 16; m++) {
                        int temp = chunk[k].blocks[i][l][m];
                        if(i == 127 && l == 15 && m > 14) {
                            chunkFileWrite << temp;
                        }
                        else {
                            chunkFileWrite << temp << " ";
                        }
                    }
                }
            }
            chunkFileWrite << "%\n";
            counter++;
        }
        chunkFileWrite.close();
        // auto generatorEnd = std::chrono::high_resolution_clock::now();
        // auto duration = duration_cast<std::chrono::milliseconds>(generatorEnd - generatorStart);
        // std::cout << "Saving time: " << duration.count() << std::endl;
        std::cout << std::endl;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////


    std::string data;
    bool startLoading = false;
    std::vector<std::vector<int>> loadedChunks;
    std::vector<int> loadedBlocks;
    std::vector<int> loadedChunkPos;
    bool start = false;
    std::string seedLoading = "";
    
    std::ifstream chunkFileRead(mapFileName + ".txt");

    while(std::getline(chunkFileRead, data)) {
        for(int i = 0; i < data.size(); i++) {
            if(data[i] == '-') {
                start = true;
            }
            if(!start) {
                seedLoading += data[i];
            }
            if(start) {
                if(data[i] == '-') {
                    int counter = 1;
                    std::string tempS = "";
                    while(data[i + counter] != '~') {
                        tempS += data[i + counter];
                        counter++;
                    }
                        std::cout << "Loading chunks: " << round((float)std::stoi(tempS)/(float)(chunkCount-1)*100) << "%" << "\t\r" << std::flush;
                        startLoading = true;
                }
                if(data[i] == '~') {
                    std::string tempPos = "";
                    for(int f = 0; f < 32; f++) {
                        if(data[i+f+1] == ' ') {
                            break;
                        }
                        tempPos += data[i+f+1];
                    }
                    loadedChunkPos.push_back(std::stoi(tempPos));
                }
                if(data[i] == '%') {
                    startLoading = false;
                    loadedChunks.push_back(loadedBlocks);
                    loadedBlocks.clear();
                }
                if(startLoading) {
                    if(data[i] == ' ') {
                        std::string tempBlock = "";
                        for(int j = 0; j < 7; j++) {
                            tempBlock += data[i+j+1];
                        }
                        loadedBlocks.push_back(std::stoi(tempBlock));
                    }
                }
            }
        }
    }
    std::cout << std::endl;
    seed = std::stoi(seedLoading);

    for(int i = 0; i < loadedChunks.size(); i++) {
        int i128 = 0;
        int l16 = 0;
        int m16 = 0;
        for(int data : loadedChunks[i]) {
            if(m16 == 16) {
                m16 = 0;
                l16++;
            }
            if(l16 == 16) {
                l16 = 0;
                i128++;
            }
            chunk[i].blocks[i128][l16][m16] = data;
            m16++;
        }
    }
    chunkFileRead.close();

    std::vector<int> blocks;
    bool side1 = false, side2 = false, side3 = false, side4 = false, top = false, bottom = false, checkBlock = false;
    for(int k = 0; k < chunkCount; k++) {
        std::cout << "Deleting invisible blocks: " << round((float)k/(float)(chunkCount-1)*100) << "%" << "\t\r" << std::flush;
        for(int i = 0; i < 128; i++) {
            for(int l = 0; l < 16; l++) {
                for(int m = 0; m < 16; m++) {
                    if(m != 15 && chunk[k].blocks[i][l][m+1] != 0 && GetBlockId(chunk[k].blocks[i][l][m+1]) != texturesDict.at("plantGrass_block")) {
                        side1 = true;
                    }
                    if(m != 0  && chunk[k].blocks[i][l][m-1] != 0 && GetBlockId(chunk[k].blocks[i][l][m-1]) != texturesDict.at("plantGrass_block")){
                        side2 = true;
                    }
                    if(l != 15 && chunk[k].blocks[i][l+1][m] != 0 && GetBlockId(chunk[k].blocks[i][l+1][m]) != texturesDict.at("plantGrass_block")) {
                        side3 = true;
                    }
                    if(l != 0  && chunk[k].blocks[i][l-1][m] != 0 && GetBlockId(chunk[k].blocks[i][l-1][m]) != texturesDict.at("plantGrass_block")) {
                        side4 = true;
                    }
                    if(i != 127 && chunk[k].blocks[i+1][l][m] != 0 && GetBlockId(chunk[k].blocks[i+1][l][m]) != texturesDict.at("plantGrass_block")) {
                        top = true;
                    }
                    if(i != 0 && chunk[k].blocks[i-1][l][m] != 0 && GetBlockId(chunk[k].blocks[i-1][l][m]) != texturesDict.at("plantGrass_block")) {
                        bottom = true;
                    }
                    /////deletes lowest layer
                    if(i == 0 && chunk[k].blocks[i+1][l][m] != 0 && GetBlockId(chunk[k].blocks[i+1][l][m]) != texturesDict.at("plantGrass_block")) {
                        bottom = true;
                    }
                    /////////////////////////
                    if(chunk[k].blocks[i][l][m] != 0) {
                        checkBlock = true;
                    }

                    if(l == 0 && (k % chunkLine) != 0 && chunk[k - 1].blocks[i][l + 15][m] != 0 && GetBlockId(chunk[k - 1].blocks[i][l + 15][m]) != texturesDict.at("plantGrass_block")) {
                        side4 = true;
                    }
                    if(l == 15 && (k % chunkLine) != (chunkLine - 1) && chunk[k + 1].blocks[i][l - 15][m] != 0 && GetBlockId(chunk[k + 1].blocks[i][l - 15][m]) != texturesDict.at("plantGrass_block")) {
                        side3 = true;
                    }
                    if(m == 0 && (k - chunkLine+1) > 0 && chunk[k - chunkLine].blocks[i][l][m + 15] != 0 && GetBlockId(chunk[k - chunkLine].blocks[i][l][m + 15]) != texturesDict.at("plantGrass_block")) {
                        side2 = true;
                    }
                    if(m == 15 && (k + chunkLine) < chunkCount && chunk[k + chunkLine].blocks[i][l][m - 15] != 0 && GetBlockId(chunk[k + chunkLine].blocks[i][l][m - 15]) != texturesDict.at("plantGrass_block")) {
                        side1 = true;
                    }
                    if(!side1 || !side2 || !side3 || !side4 || !top || !bottom) {
                        if(checkBlock) {
                            blocks.push_back(chunk[k].blocks[i][l][m]);
                            blockCounter++;
                        }
                    }
                    side1 = false, side2 = false, side3 = false, side4 = false, top = false, bottom = false, checkBlock = false;
                }
            }
        }
        numberOfBlocksInChunk.nuberOfBlocks[k] = blockCounter;
        blockCounter = 0;
    }
    std::cout << std::endl;
    chunkToGPU.resize(chunkCount);
    std::memset(chunkToGPU.data(), 0, sizeof(ChunkToGPU) * chunkToGPU.size());

    int counter = 0;
    for(int c = 0; c < chunkCount; c++) {
        for(int i = 0; i < numberOfBlocksInChunk.nuberOfBlocks[c]; i++) {
            chunkToGPU[c].blocks[i] = blocks[i + counter];
        }
        counter += numberOfBlocksInChunk.nuberOfBlocks[c];
        chunkToGPU[c].chunkPos = loadedChunkPos[c];
    }
    /////////////////////////////////////////////////////////////////////////////////
    for(int i = 0; i < sizeof(numberOfBlocksInChunk.nuberOfBlocks)/sizeof(numberOfBlocksInChunk.nuberOfBlocks[0]); i++) {
        blockCounter += numberOfBlocksInChunk.nuberOfBlocks[i];
    }
}