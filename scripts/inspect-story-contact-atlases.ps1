$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
public static class StoryContactBounds {
    public static string Read(string path, string name, bool contact) {
        using(var image=new Bitmap(path)) {
            if(image.Width!=1536 || image.Height!=1024 || image.GetPixel(0,0).A>16)
                throw new Exception("Invalid story atlas: "+path);
            string result="inline constexpr AtlasCrop "+name+"[] = {\n";
            for(int frame=0;frame<4;++frame) {
                int left=(frame%2)*768, top=(frame/2)*512, width=768, height=512;
                if(contact) {
                    int[,] boxes={{0,0,680,500},{680,0,856,500},{0,512,768,512},{768,496,768,528}};
                    left=boxes[frame,0];top=boxes[frame,1];width=boxes[frame,2];height=boxes[frame,3];
                }
                int x0=left+width,y0=top+height,x1=left,y1=top;
                for(int y=top;y<top+height;++y) for(int x=left;x<left+width;++x)
                    if(image.GetPixel(x,y).A>180) {
                        x0=Math.Min(x0,x); y0=Math.Min(y0,y);
                        x1=Math.Max(x1,x); y1=Math.Max(y1,y);
                    }
                if(x1<=x0 || y1<=y0) throw new Exception("Empty frame");
                x0=Math.Max(left,x0-2); y0=Math.Max(top,y0-2);
                x1=Math.Min(left+width-1,x1+2); y1=Math.Min(top+height-1,y1+2);
                result+="    {"+x0+","+y0+","+(x1-x0+1)+","+(y1-y0+1)+"},\n";
            }
            return result+"};\n";
        }
    }
}
'@
$root=Split-Path $PSScriptRoot
$header="#pragma once`n#include `"CharacterAtlas.h`"`n// Alpha bounds only; generated images are not modified.`n"
$header += [StoryContactBounds]::Read((Join-Path $root 'visual/Content/art/story-interactions-v2.png'),'StoryInteractionCrops',$true)
$header += [StoryContactBounds]::Read((Join-Path $root 'visual/Content/art/bento-guardian-run.png'),'BentoGuardianCrops',$false)
$header += [StoryContactBounds]::Read((Join-Path $root 'visual/Content/art/jaca-bite-sequence.png'),'JacaBiteCrops',$false)
$header += [StoryContactBounds]::Read((Join-Path $root 'visual/Content/art/story-arrival-v1.png'),'StoryArrivalCrops',$false)
[IO.File]::WriteAllText((Join-Path $root 'visual/Source/StoryContactAtlas.h'),$header,[Text.UTF8Encoding]::new($false))
Write-Output $header
