#version 330 core
out vec4 color;

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoord;
  
uniform vec3 lightPos; 
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform sampler2D ourTexture;

uniform float ka;
uniform float kd;
uniform float ks;

void main()
{

	// Ambient
    vec3 ambient = ka * lightColor;
  	
    // Diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = kd * diff * lightColor;
    
    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = ks * spec * lightColor;  

	vec4 asd = texture(ourTexture, TexCoord);
        
    vec4 result = vec4((ambient + diffuse + specular),1.0) * asd;
    color = result;
} 

