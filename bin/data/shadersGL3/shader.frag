#version 400

flat in vec4 hexColor;
//in vec4 hexColor;
out vec4 out_color;

void main()
{
    out_color = vec4( hexColor.rgb, 1.0 );
}
