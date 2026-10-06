# Export launcher sizes from the original transparent mascot; no generative edits.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;
using System.Collections.Generic;

public static class JacaIconExport {
    static Rectangle Bounds(Bitmap source) {
        int left=source.Width, top=source.Height, right=-1, bottom=-1;
        for(int y=0;y<source.Height;y++) for(int x=0;x<source.Width;x++) {
            if(source.GetPixel(x,y).A<8) continue;
            left=Math.Min(left,x); top=Math.Min(top,y);
            right=Math.Max(right,x); bottom=Math.Max(bottom,y);
        }
        if(right<left) throw new InvalidDataException("Empty icon artwork");
        return Rectangle.FromLTRB(left,top,right+1,bottom+1);
    }
    static Bitmap Render(Bitmap source, Rectangle crop, int size, float fraction, string shape) {
        Bitmap result=new Bitmap(size,size,PixelFormat.Format32bppArgb);
        using(Graphics g=Graphics.FromImage(result)) {
            g.SmoothingMode=SmoothingMode.AntiAlias;
            g.InterpolationMode=InterpolationMode.HighQualityBicubic;
            g.PixelOffsetMode=PixelOffsetMode.HighQuality;
            g.Clear(Color.Transparent);
            if(shape!="transparent") {
                using(GraphicsPath mask=new GraphicsPath()) {
                    if(shape=="round") mask.AddEllipse(0,0,size,size);
                    else if(shape=="square") mask.AddRectangle(new Rectangle(0,0,size,size));
                    else {
                        float d=size*.42f;
                        mask.AddArc(0,0,d,d,180,90); mask.AddArc(size-d,0,d,d,270,90);
                        mask.AddArc(size-d,size-d,d,d,0,90); mask.AddArc(0,size-d,d,d,90,90);
                        mask.CloseFigure();
                    }
                    using(SolidBrush background=new SolidBrush(Color.FromArgb(11,52,53))) g.FillPath(background,mask);
                    g.SetClip(mask);
                }
            }
            float scale=size*fraction/Math.Max(crop.Width,crop.Height);
            float w=crop.Width*scale, h=crop.Height*scale;
            g.DrawImage(source,new RectangleF((size-w)/2,(size-h)/2,w,h),crop,GraphicsUnit.Pixel);
        }
        return result;
    }
    static void Save(Bitmap source, Rectangle crop, string file, int size, float fraction, string shape) {
        Directory.CreateDirectory(Path.GetDirectoryName(file));
        using(Bitmap icon=Render(source,crop,size,fraction,shape)) icon.Save(file,ImageFormat.Png);
    }
    public static void Export(string root) {
        string art=Path.Combine(root,"assets/branding");
        string res=Path.Combine(root,"visual/proj.android/app/res");
        using(Bitmap source=new Bitmap(Path.Combine(art,"jaca-icon-source.png"))) {
            Rectangle crop=Bounds(source);
            string[] densities={"mdpi","hdpi","xhdpi","xxhdpi","xxxhdpi"};
            int[] sizes={48,72,96,144,192};
            for(int i=0;i<sizes.Length;i++) {
                string dir=Path.Combine(res,"mipmap-"+densities[i]);
                Save(source,crop,Path.Combine(dir,"ic_launcher.png"),sizes[i],.88f,"rounded");
                Save(source,crop,Path.Combine(dir,"ic_launcher_round.png"),sizes[i],.84f,"round");
            }
            // 108dp at xxxhdpi. The complete mascot fits inside the central 64dp.
            Save(source,crop,Path.Combine(res,"drawable-xxxhdpi/ic_launcher_foreground.png"),432,64f/108,"transparent");
            Save(source,crop,Path.Combine(art,"jaca-icon-512.png"),512,.88f,"square");
            Save(source,crop,Path.Combine(art,"jaca-icon-rounded.png"),512,.88f,"rounded");
            Save(source,crop,Path.Combine(art,"jaca-icon-round.png"),512,.84f,"round");

            // One ICO containing 16/24/32/48/64/128/256px PNG frames for Windows.
            int[] frames={16,24,32,48,64,128,256};
            List<byte[]> encoded=new List<byte[]>();
            foreach(int size in frames) using(Bitmap icon=Render(source,crop,size,.88f,"rounded"))
                using(MemoryStream stream=new MemoryStream()) { icon.Save(stream,ImageFormat.Png); encoded.Add(stream.ToArray()); }
            using(BinaryWriter writer=new BinaryWriter(File.Create(Path.Combine(root,"visual/proj.win32/res/game.ico")))) {
                writer.Write((ushort)0); writer.Write((ushort)1); writer.Write((ushort)frames.Length);
                int offset=6+16*frames.Length;
                for(int i=0;i<frames.Length;i++) {
                    writer.Write((byte)(frames[i]==256?0:frames[i])); writer.Write((byte)(frames[i]==256?0:frames[i]));
                    writer.Write((byte)0); writer.Write((byte)0); writer.Write((ushort)1); writer.Write((ushort)32);
                    writer.Write(encoded[i].Length); writer.Write(offset); offset+=encoded[i].Length;
                }
                foreach(byte[] bytes in encoded) writer.Write(bytes);
            }
            // Preview actual adaptive masks: Android exposes the central 72dp of 108dp.
            using(Bitmap preview=new Bitmap(680,380)) using(Graphics g=Graphics.FromImage(preview)) {
                g.Clear(Color.FromArgb(240,241,224));
                using(Bitmap a=Render(source,crop,256,64f/72,"rounded")) g.DrawImageUnscaled(a,50,28);
                using(Bitmap b=Render(source,crop,256,64f/72,"round")) g.DrawImageUnscaled(b,374,28);
                using(Bitmap a=Render(source,crop,48,.88f,"rounded")) g.DrawImageUnscaled(a,154,306);
                using(Bitmap b=Render(source,crop,48,.84f,"round")) g.DrawImageUnscaled(b,478,306);
                preview.Save(Path.Combine(root,"docs/images/app-icon-preview.png"),ImageFormat.Png);
            }
            Console.WriteLine("Icon exports complete. Source alpha bounds: "+crop);
        }
    }
}
'@
[JacaIconExport]::Export((Split-Path $PSScriptRoot -Parent))
