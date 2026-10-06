# Original synthesized effects for JacaRun. No samples or external assets.
$ErrorActionPreference = 'Stop'
$destination = Join-Path $PSScriptRoot '../visual/Content/audio'
New-Item -ItemType Directory -Path $destination -Force | Out-Null
$sounds = @(
    @{Name='jump'; Duration=.20; Start=260; End=620},
    @{Name='slide'; Duration=.18; Start=260; End=90},
    @{Name='coin'; Duration=.12; Start=1000; End=1500},
    @{Name='food'; Duration=.20; Start=620; End=940},
    @{Name='hit'; Duration=.25; Start=155; End=45},
    @{Name='reward'; Duration=.54; Start=523; End=1046}
)
foreach ($sound in $sounds) {
    $rate = 22050
    $count = [int]($rate * $sound.Duration)
    $stream = [IO.File]::Create((Join-Path $destination ($sound.Name + '.wav')))
    $writer = [IO.BinaryWriter]::new($stream)
    try {
        $writer.Write([Text.Encoding]::ASCII.GetBytes('RIFF'))
        $writer.Write([int](36 + $count * 2))
        $writer.Write([Text.Encoding]::ASCII.GetBytes('WAVEfmt '))
        $writer.Write([int]16); $writer.Write([int16]1); $writer.Write([int16]1)
        $writer.Write([int]$rate); $writer.Write([int]($rate * 2))
        $writer.Write([int16]2); $writer.Write([int16]16)
        $writer.Write([Text.Encoding]::ASCII.GetBytes('data')); $writer.Write([int]($count * 2))
        $phase = 0.0
        for ($i = 0; $i -lt $count; $i++) {
            $t = $i / [double]$count
            $frequency = $sound.Start + ($sound.End - $sound.Start) * $t
            if ($sound.Name -eq 'reward') {
                $frequency = @(523.25,659.25,783.99,1046.5)[[Math]::Min(3,[int][Math]::Floor($t*4))]
            }
            $phase += 2 * [Math]::PI * $frequency / $rate
            $envelope = [Math]::Min(1,$t*30) * [Math]::Pow(1-$t,1.7)
            $sample = ([Math]::Sin($phase) + .18 * [Math]::Sin(2*$phase)) * $envelope * 10000
            $writer.Write([int16]$sample)
        }
    } finally { $writer.Dispose() }
}
Write-Output 'Generated six original PCM WAV effects.'
