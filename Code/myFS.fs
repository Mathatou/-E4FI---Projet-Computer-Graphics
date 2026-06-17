#version 330 core

in vec3 v_Normal; 
in vec3 v_Position;
in vec2 v_texCoord;

out vec4 FragColor;

const vec3 LightDirection = vec3(1.0, -1.0, -1.0);
const vec3 SkyVec = vec3(0.0,1.0,0.0);
const float ambientIntensity = 0.7;


struct u_Material
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform u_Material u_mat ;
uniform sampler2D m_sampler;
uniform int u_hasTexture;
uniform int u_reflectivityPreset; // 0 = Or, 1 = Eau, 2 = Argent
uniform vec3 u_Reflectivity = vec3(0.04, 0.04, 0.04); // Réflectivité par défaut pour les matériaux non métalliques


layout(std140) uniform CameraData
{
    mat4 m_ViewMatrix;
    mat4 m_Perspective;
};

vec3 diffuse(vec3 N, vec3 L, vec3 color,float Id)
{
    float NL = max(dot(N, L), 0.0);

    vec3 RGB = color * NL * Id;

    return RGB;
}

vec3 specular(vec3 N, vec3 L, vec3 V, float Id )
{
    vec3 R = reflect(-L, N);
    float specfactor = pow(max(dot(R,V),0.0),u_mat.shininess) * Id;
    return u_mat.specular * specfactor;
}

vec3 BP_specular(vec3 N, vec3 L, float Id )
{
    vec3 V = vec3(0.0) - normalize(v_Position);
    vec3 H = normalize((L+V));
    float specularfactor = pow(max(dot(N,H),0.0),u_mat.shininess) * Id;
    vec3 RGB = u_mat.specular * specularfactor;

    return RGB;
}


void main(void) {
    // Lecture texture
    vec4 color;
    if(u_hasTexture == 1)
        color = texture(m_sampler,v_texCoord);
    else
        color = vec4(u_mat.diffuse, 1.0);

    vec4 colorTotal = color;
    // Config vecs
    vec3 N = normalize(v_Normal); // normale en repere monde 
    vec3 L = normalize(-LightDirection); // direction VERS la lumiere
    float Id = 1.0; // Intensite de la lumiere
    // Calcul vecteur caméra
    vec3 camPos = inverse(m_ViewMatrix)[3].xyz;
    vec3 V = normalize(camPos - v_Position);
    // Calcul lumiere
    vec3 diffuseColor  = diffuse(N, L,colorTotal.rgb, Id );
    vec3 specularColor = specular(N, L, V, Id);
    //vec3 BPSpecularColor = BP_specular(N,L ,Id);
    //vec3 ambientColor = vec3(ambientIntensity) * colorTotal.rgb;

    // Effet Fresnel
    float cosTheta = max(dot(N, V), 0.0); // angle regard vs surface
    vec3 reflectivity; // réflectivité de la surface
    switch(u_reflectivityPreset) 
    {
        case 1: // Or
            reflectivity = vec3(1.0, 0.71, 0.29);
            break;
        case 2: // Eau
            reflectivity = vec3(0.02, 0.04, 0.08);
            break;
        case 3: // Argent
            reflectivity = vec3(0.91, 0.92, 0.92);
            break;
        case 4: // Bronze
            reflectivity = vec3(0.8, 0.5, 0.2);
            break;
        default:
            reflectivity = u_Reflectivity; // Utiliser la valeur par défaut si aucun preset n'est sélectionné
            break;
    }
    vec3 fresnelColor = reflectivity + (vec3(1.0) - reflectivity) * pow(1.0 - cosTheta, 5.0);
    // conversion
    vec3 kD = vec3(1.0) - fresnelColor;

    float NdotSky = dot(N,SkyVec);
    vec3 SkyColor = vec3(0.7,0.0,1.0);
    vec3 GroundColor = vec3(0.3,1.0,0.0);

    float Hemispherefactor = NdotSky *0.5 +0.5;
    vec3 ambiant = u_mat.ambient * colorTotal.rgb * mix(GroundColor,SkyColor,Hemispherefactor);
    //Mix
    vec3 PhongIllumination = ambiant + (diffuseColor * kD) + (specularColor * fresnelColor);
    FragColor = vec4(PhongIllumination, 1.0);
}