#include <iostream>
#include <fstream>
#include <random>
#include "glad/glad.h"
#define SDL_MAIN_HANDLED_H
#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>
#include <string.h>
#include "Constants.h"
#include "GPU_bundle.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "camera.h"
#include <thread>
#include <time.h>


using json = nlohmann::json_abi_v3_12_0::json;


//__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;

void input(SDL_Event &event_buffer, SDL_Window *Wind, bool &running){
    while(SDL_PollEvent(&event_buffer)){
        switch(event_buffer.type){
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            SDL_DestroyWindow(Wind);
            running = false;
            break;
        }
    }
}

struct Person{
    //Json
    uint32_t id;
    std::string Name;
    std::unordered_map<uint32_t, float> relations;

    //physics
    glm::vec2 position;
    glm::vec3 color = glm::vec3(1.f);

    void load_from_json(std::unordered_map<std::string, uint32_t>& name_id_table,json& Loadfile){
        id = Loadfile["id"].get<uint32_t>();
        Name = Loadfile["name"].get<std::string>();
        for(const auto& x: Loadfile["relations"].items()){
            relations[name_id_table[x.key()]] = x.value();
        }
    }
    Person(){
        position.x = (static_cast<float>(rand()%(2*WORLD_LIMIT))/100.f)-WORLD_LIMIT/100.f;
        position.y = (static_cast<float>(rand()%(2*WORLD_LIMIT))/100.f)-WORLD_LIMIT/100.f;
    }
};
struct alignas(16) GPU_Person{
    uint32_t id;
    uint32_t relation_offset;
    uint32_t relation_count;
    uint32_t padding;
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec2 acceleration = glm::vec2(0.f);
    glm::vec2 padding2;
    glm::vec4 color;
    GPU_Person(){}
    GPU_Person(Person person){
        position = person.position;
        id = person.id;
        color = glm::vec4(person.color,1.f);
    }
};



int main(){
    std::random_device rd;
    std::srand(std::time(NULL));

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    SDL_Window* Wind = SDL_CreateWindow("kocapopo",resolution[0],resolution[1],SDL_WINDOW_OPENGL);

    SDL_GLContext noname = SDL_GL_CreateContext(Wind);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);

    gladLoadGL();

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "GLAD load failed\n";
        return -1;
    }

    glm::mat4 projection = glm::ortho(
        -aspect, aspect,  // left, right
        -1.0f, 1.0f,      // bottom, top
        -1.0f, 1.0f       // near, far
    );

    Camera main_camera;

    main_camera.position = glm::vec2(0.0f,0.f);
    main_camera.direction = glm::vec3(0.f,0.f,-1.f);
    main_camera.speed = 1.f;
    glm::mat4 view = main_camera.get_camera_update();

    bool running = true;

    std::ifstream relation_map_file("people_1000.json");
    json relation_map;

    relation_map_file >> relation_map;

    SDL_Event event_buffer;

    Shader Vertex_Shader(GL_VERTEX_SHADER);
    Shader Fragment_Shader(GL_FRAGMENT_SHADER);
    Shader_program Render_prog;

    Vertex_Shader.get_shader("shaders/Shader.vert");
    Fragment_Shader.get_shader("shaders/Shader.frag");
    Render_prog.Add_Shader(Vertex_Shader);
    Render_prog.Add_Shader(Fragment_Shader);

    Shader_program physics_prog;

    Shader Force_Compute_Shader(GL_COMPUTE_SHADER);
    Force_Compute_Shader.get_shader("shaders/Forces.comp");
    physics_prog.Add_Shader(Force_Compute_Shader);

    glLinkProgram(Render_prog.program_id);
    GLint success;
    GLchar infoLog[1024];
    glGetProgramiv(Render_prog.program_id, GL_LINK_STATUS, &success);
    if(!success){
        glGetProgramInfoLog(Render_prog.program_id, 1024, NULL, infoLog);
        std::cout << "Link error: " << infoLog << "\n";
    }

    glLinkProgram(physics_prog.program_id);
    glGetProgramiv(physics_prog.program_id, GL_LINK_STATUS, &success);
    if(!success){
        glGetProgramInfoLog(physics_prog.program_id, 1024, NULL, infoLog);
        std::cout << "Link error: " << infoLog << "\n";
    }

    std::cout << sizeof(GPU_Person) << "\n";

    GLint delta_time_loc = glGetUniformLocation(physics_prog.program_id,"delta_time");
    GLint total_people_loc = glGetUniformLocation(physics_prog.program_id,"total_people");

    Vertex_Shader.destroy();
    Fragment_Shader.destroy();
    Force_Compute_Shader.destroy();

    glUseProgram(Render_prog.program_id);

    GLint proj_loc = glGetUniformLocation(Render_prog.program_id,"proj");
    glUniformMatrix4fv(proj_loc,1,false,glm::value_ptr(projection));
    GLint view_loc = glGetUniformLocation(Render_prog.program_id,"view");
    glUniformMatrix4fv(view_loc,1,false,glm::value_ptr(view));

    Mesh person_mesh = create_circle_mesh(0.5f,10);

    uint64_t time_buffer;
    float delta_time;
    time_buffer = SDL_GetTicksNS();

    constexpr float Target_FPNS = 1000000000.f/FPS;

    uint32_t calculatable_people_size;

    std::vector<Person> people;
    std::vector<GPU_Person> people_calculatable;

    people.resize(relation_map["Persons"].size());
    std::unordered_map<std::string, uint32_t> name_to_id_table;
    uint32_t person_count = relation_map["Persons"].size();
    uint32_t max_relations = 0;
    uint32_t min_relations = inf;
    uint32_t total_relations = 0;

    GPU_Object drawed_obj;
    create_object_from_circular_mesh(person_mesh,&drawed_obj);

    for(json& x: relation_map["Persons"]){
        name_to_id_table[x["name"]] = x["id"];
    }
    for(int x = 0;x<relation_map["Persons"].size();x++){
        people[x].load_from_json(name_to_id_table,relation_map["Persons"][x]);
        max_relations = std::max(max_relations,static_cast<uint32_t>(relation_map["Persons"][x]["relations"].size()));
        min_relations = std::min(min_relations,static_cast<uint32_t>(relation_map["Persons"][x]["relations"].size()));
        total_relations += relation_map["Persons"][x]["relations"].size();
    }

    uint32_t idlist[total_relations];
    float rellist[total_relations];
    size_t offset_buff = 0;
    size_t list_buff = 0;
    for(int x = 0;x<people.size();x++){
        people_calculatable.push_back(GPU_Person(people[x]));
        people_calculatable[x].relation_count = people[x].relations.size();
        people_calculatable[x].relation_offset = offset_buff;
        offset_buff += people[x].relations.size();

        for(auto& [id,value]: people[x].relations){
            idlist[list_buff] = id;
            rellist[list_buff] = value;
            list_buff++;
        }

        std::cout << people[x].id <<" " << people[x].position.x << " " << people[x].position.y << "\n";
    }

    std::cout << max_relations << "\n";
    std::cout << min_relations << "\n\n";

    std::vector<GPU_Person> empty_people;
    empty_people.resize(person_count);

    glUseProgram(physics_prog.program_id);

    glUniform1ui(total_people_loc,relation_map["Persons"].size());

    GLuint ssbo[4];
    glGenBuffers(4,ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,ssbo[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER,people_calculatable.size()*sizeof(GPU_Person),people_calculatable.data(),GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,ssbo[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER,total_relations*sizeof(uint32_t),idlist,GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,ssbo[2]);
    glBufferData(GL_SHADER_STORAGE_BUFFER,total_relations*sizeof(float),rellist,GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,ssbo[3]);
    glBufferData(GL_SHADER_STORAGE_BUFFER,sizeof(GPU_Person)*person_count,empty_people.data(),GL_DYNAMIC_COPY);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,ssbo[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,ssbo[1]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,ssbo[2]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,ssbo[3]);

    glm::vec2 mouse_pos_past_buff;
    glm::vec2 mouse_pos_buff;

    bool mouse_panning = false;
    glm::vec2 camera_position_buffer;

    float mouse_zoom = 1.f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    while(running){
    //input

        while(SDL_PollEvent(&event_buffer)){
            switch(event_buffer.type){
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                SDL_DestroyWindow(Wind);
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if(event_buffer.key.key == SDLK_LEFT)
                    main_camera.Camera_move(delta_time*glm::vec2(-0.2f,0.f));

                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if(event_buffer.button.button == SDL_BUTTON_MIDDLE){
                    mouse_panning = true;
                }
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                if(event_buffer.button.button == SDL_BUTTON_MIDDLE){
                    mouse_panning = false;
                }
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                if(event_buffer.wheel.y>0){
                    mouse_zoom*=0.9f;
                }
                if(event_buffer.wheel.y<0){
                    mouse_zoom*=1.1f;
                }
                break;
            }
        }

        glm::mat4 projection = glm::ortho(
            -aspect*mouse_zoom, aspect*mouse_zoom,  // left, right
            -1.0f*mouse_zoom, 1.0f*mouse_zoom,      // bottom, top
            -1.0f, 1.0f       // near, far
        );

        //std::cout << time_buffer << " " << delta_time << " " << 1/delta_time <<"\n";

        glClearColor(0.1f,0.1f,0.1f,1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniformMatrix4fv(proj_loc,1,false,glm::value_ptr(projection));
        glUniformMatrix4fv(view_loc,1,false,glm::value_ptr(view));

        glUseProgram(physics_prog.program_id);

        glUniform1f(delta_time_loc, delta_time);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo[0]); // read
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo[1]); // ids
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo[2]); // values
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo[3]); // write
        glDispatchCompute((person_count+255)/256, 1, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        std::swap(ssbo[0], ssbo[3]);

        glUseProgram(Render_prog.program_id);
        main_camera.get_camera_update();
        view = main_camera.get_camera_update();

        glBindVertexArray(drawed_obj.vao);
        glDrawElementsInstanced(GL_TRIANGLES,drawed_obj.index_count,GL_UNSIGNED_INT,nullptr,person_count);

        SDL_GL_SwapWindow(Wind);

        SDL_DelayPrecise((Target_FPNS-delta_time));
        delta_time = static_cast<float>(SDL_GetTicksNS()-time_buffer);
        time_buffer = SDL_GetTicksNS();

        SDL_GetMouseState(&mouse_pos_buff.x,&mouse_pos_buff.y);

        if(!mouse_panning){
            mouse_pos_past_buff = mouse_pos_buff;
            camera_position_buffer = main_camera.position;
        }

        else{
            main_camera.position.x=camera_position_buffer.x + 2*((mouse_pos_past_buff-mouse_pos_buff).x/resolution[1])*mouse_zoom; //neden bunun düzelttiğini bilmiyorum
            main_camera.position.y=camera_position_buffer.y - 2*((mouse_pos_past_buff-mouse_pos_buff).y/resolution[1])*mouse_zoom;
        }

        delta_time/=1000000000.f;

    }

    SDL_Quit();
    std::_Exit(0);
}
