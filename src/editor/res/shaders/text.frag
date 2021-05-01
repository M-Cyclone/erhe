in vec2 v_texcoord;
in vec4 v_color;

void main(void)
{
    //out_color.r = (v_color * texture(s_texture, v_texcoord)).r;
    vec2  c       = texture(s_texture, v_texcoord).rg; //texture(font_texture, v_texcoord.xy).rg;
    float inside  = c.r;
    float outline = c.g;
    float alpha   = max(inside, outline * 0.25);
    vec3  color   = v_color.rgb * inside; // pow(inside, 0.5);

    out_color = vec4(color, alpha);
}
