#version 120

varying vec3 v_Normal; 
varying vec3 v_Position;
varying vec2 v_texCoord;
const vec3 LightDirection = vec3(1.0, -1.0, -1.0);
const vec3 SkyVec = vec3(0.0,1.0,0.0);
const float ambientIntensity = 0.7;


struct u_Material
{
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform u_Material u_mat ;
uniform sampler2D m_sampler;
uniform int u_hasTexture;

vec3 diffuse(vec3 N, vec3 L, vec3 color,float Id)
{
    float NL = max(dot(N, L), 0.0);

    vec3 RGB = color * NL * Id;

    return RGB;
}

vec3 specular(vec3 N, vec3 L, float Id )
{
    vec3 R = reflect(-L, N);
    vec3 V = vec3(0.0) - normalize(v_Position);
    float specularfactor = pow(max(dot(R,V),0.0),u_mat.shininess) * Id;

    vec3 RGB = u_mat.specular * specularfactor;

    return RGB;
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
        color = texture2D(m_sampler,v_texCoord);
    else
        color = vec4(u_mat.diffuse, 1.0);

    vec4 colorTotal = color;
    // Config vecs
    vec3 N = normalize(v_Normal); // normale en repere monde 
    vec3 L = normalize(-LightDirection); // direction VERS la lumiere
    float Id = 1.0; // Intensite de la lumiere
    // Calcul lumiere
    vec3 diffuseColor  = diffuse(N, L,colorTotal.rgb, Id );
    vec3 specularColor = specular(N,L ,Id);
    //vec3 ambientColor = vec3(ambientIntensity) * colorTotal.rgb;
    
    float NdotSky = dot(N,SkyVec);
    vec3 SkyColor = vec3(0.0,0.0,1.0);
    vec3 GroundColor = vec3(0.0,1.0,0.0);

    float Hemispherefactor = NdotSky *0.5 +0.5;
    vec3 ambiant = ambientIntensity * colorTotal.rgb * mix(SkyColor,GroundColor,Hemispherefactor);
    //Mix
    vec3 PhongIllumination = ambiant + diffuseColor + specularColor;
    gl_FragColor = vec4(PhongIllumination, 1.0);
}