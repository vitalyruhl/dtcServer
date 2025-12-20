
<#______________________________________________________________________________________________________________________

	(c) Vitaly Ruhl 2021
______________________________________________________________________________________________________________________#>

$Funktion = 'backup.ps1'

  
<#______________________________________________________________________________________________________________________    
    		Version  	Datum           Author        Beschreibung
    		-------  	----------      -----------   -----------                                                       #>

$Version = 100 #	03.08.2021		Vitaly Ruhl		init
$Version = 110 #	13.09.2022		Vitaly Ruhl		Add Robocopy Option
$Version = 111 #	13.09.2022		Vitaly Ruhl		Bugfix on source as drive like s:\
$Version = 120 #	13.09.2022		Vitaly Ruhl		Add aditional BackupSettings.json 


<#______________________________________________________________________________________________________________________
    Function:
    Make backup of all Files in contained folder, except exclusions...
______________________________________________________________________________________________________________________#>


<#______________________________________________________________________________________________________________________
    To-Do / Errors:
        03.08.2021 Exclusion don't work with kompressing -> its copy all file (not implemented yet)
        03.08.2021 If there no sub-Folder, the script will not working (add the Path not automaticly)
______________________________________________________________________________________________________________________#>


#**********************************************************************************************************************
# Settings
    $AdminRightsRequired = $false #set $true, if Admin-Rights are for the Script reqired
    $Prefix = '' #'zzz_' #Prefix of Backup-Folder

    #$AD = Get-Date -Format yyyy.MM.dd_HH-mm
    $Sufix = '' #"_Bakup_$AD"  #Sufix of Backup-Folder
    
    # Result: zzz_ContainedFolder_Bakup_2021.08.03_22-18

    $CompressIntoTargetPaths = $false # Save compressed file instead of uncompressed Folder !! Not with Option UseRobocopy !!

    # 13.09.2022 Robocopy Region
        $UseRobocopy = $true 
        $Parameter = "/J /MIR /R:2 /W:1 /NP" 
        $ExcludePath = '' # '/XD D:\`$RECYCLE.BIN "System Volume Information" "RECYCLER"' #/XD exclude-fold* | /XF "C:\source\folder\path\to\folder\filename.extension"
    # end Robocopy Region

    # An Array of Targets - Save all in more then 1 Backup

    $TargetPaths = @(
        'Y:\zz_bkp_Test\'
    )
    
    $Excludes  = @(# Working with LIKE operator -> can contains *,% etc...
        "*xvba_debug.log",  # !!!!!#xvba_debug.log must be exclude from Backup. Otherwise crashes on own debug-files
        '*`$RECYCLE.BIN',
        "System Volume Information", 
        "RECYCLER",
        "Thumbs.db"
    )

    # predefined Variable for AutoUpdate
    $UpdateVersion = 0
    $AllowUpdate = $false
    $UpdateFromPath = "\\tower\programmierung\Powerschell\Backup-V1"
    $UpdateFile = "backup.ps1"
    $UpdateVersionFile = "VersionSettings.json"

#**********************************************************************************************************************


#region Debugging Functions

    #**********************************************************************************************************************
    # Debug Settings
        $global:debug = $false # $true $false
        $global:DebugPrefix = $Funktion + ' ' + $Version + ' -> ' #Variable für Debug-log vorbelegen
        $global:Modul = 'Main' #Variable für Debug-log vorbelegen
        $ErrorActionPreference = "Continue" #(Ignore,SilentlyContinue,Continue,Stop,Inquire) 
        $global:DebugPreference = if ($global:debug) {"Continue"} else {"SilentlyContinue"} #Powershell-Own Debug settings
    #**********************************************************************************************************************

function SetDebugState ($b){
    $global:DebugPreference = if ($b) {"Continue"} else {"SilentlyContinue"} #Powershell-Own Debug settings
}

function section ($text) {
    Write-Host "`r`n-----------------------------------------------------------------------------------------------"
    Write-Host " $text"
    Write-Host "`r`n"
}
	
function sectionY ($text) {
    Write-Host "`r`n-----------------------------------------------------------------------------------------------" -ForegroundColor Yellow
    Write-Host " $text" -ForegroundColor Yellow
    Write-Host "`r`n"
}
	
function log ($text) {
    if ($global:debug) {
        Write-Host "$global:DebugPrefix $global:Modul -> $text" -ForegroundColor DarkGray	
    }
}

function debug ($text){
    if ($global:debug) {
        Write-debug "$global:DebugPrefix $global:Modul -> $text"# -ForegroundColor DarkGray
    }	
}

#endregion


#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#region Some Functions

function compress($Source,$Target)
{
    Write-Host 'Compressing...'
    #Add-Type -AssemblyName System.IO.Compression.FileSystem
    #$compressionLevel = [System.IO.Compression.CompressionLevel]::Optimal
    #[System.IO.Compression.ZipFile]::CreateFromDirectory($Source, $Target, $compressionLevel, $True)    

    if (-not (test-path "$env:ProgramFiles\7-Zip\7z.exe")) {throw "$env:ProgramFiles\7-Zip\7z.exe needed"} 
    set-alias sz "$env:ProgramFiles\7-Zip\7z.exe"  
    sz a -mx=9 "$Target" "$Source"
}


function start-countdown ($sleepintervalsec) {
	    <#
			Use: start-countdown 60
		#>
	$ec = 0
	foreach ($step in (1..$sleepintervalsec)) {
		try {
			if ([console]::KeyAvailable) {
				$key = [system.console]::readkey($true)
				if (($key.modifiers -band [consolemodifiers]"control") -and ($key.key -eq "C")) {
					Write-Warning "CTRL-C pressed" 
					return
				}
				else {
					Write-Host "Key Pressed [$($key.keychar)]"
					pause
					return
				}
			}
		}
		catch {
			if ($ec -eq 0) {
				Write-Warning "Start in Powershell ISE - console functions are not avaible"
				$ec++
			}
		}
		finally {
			$rest = $sleepintervalsec - $step
			write-progress -Activity "Please wait" -Status " $rest Sek..." -SecondsRemaining ($rest) -PercentComplete  ($step / $sleepintervalsec * 100)
			start-sleep -seconds 1
		}
	}
}

function Add-Path($MyPath) {
    #Checks path exists, otherwise creates a new one .....
    <#
               example: 
               $Pfad="$env:TEMP\PS_Skript"
               Add-Path($Pfad)
       #>
    $tempModul = $global:Modul # Save pre-text temporarily 
    $global:Modul = 'Add-Path'
   
    try {
           
        if (!(Test-Path -path $MyPath -ErrorAction SilentlyContinue )) {
            # Create Path if not exist
            if (!(Test-Path -Path $MyPath)) {
                New-Item -Path $MyPath -ItemType Directory -ErrorAction SilentlyContinue # | Out-Null
            }      
        }
   
    }
    catch { 
        Write-Warning "$global:Modul -  Something went wrong" 
    }	
    $global:Modul = $tempModul #restore old module text	
}	
function Get-ScriptDirectory() {
    $tempModul = $global:Modul # Save pre-text temporarily 
    $global:Modul = 'Get-ScriptDirectory'
    try {
        $Invocation = (Get-Variable MyInvocation -Scope 1).Value
        Split-Path $Invocation.MyCommand.Path
    }
    catch { 
        Write-Warning "$global:Modul -  Something went wrong" 
    }	
    $global:Modul = $tempModul #restore old module text	
}
	
if ($AdminRightsRequired){
    log "get Adminrights... $AdminRightsRequired"
    #+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    $identity = [System.Security.Principal.WindowsIdentity]::GetCurrent()
    $princ = New-Object System.Security.Principal.WindowsPrincipal($identity)
    if (!$princ.IsInRole([System.Security.Principal.WindowsBuiltInRole]::Administrator)) {
        $powershell = [System.Diagnostics.Process]::GetCurrentProcess()
        $psi = New-Object System.Diagnostics.ProcessStartInfo $powerShell.Path
        $script = $MyInvocation.MyCommand.Path
        $prm = $script
        foreach ($a in $args) {
            $prm += ' ' + $a
        }
        $psi.Arguments = $prm
        $psi.Verb = 'runas'
        [System.Diagnostics.Process]::Start($psi) | Out-Null
        return;
    }
    #+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

}
#endregion


# Debugging session
if ($global:debug) {
    #Clear-Host
    log "entry"
    log "module imported"
    $global:Modul = 'ENV'
    sectiony "ENV-Test"
    $PC = $env:computername

    $datum = Get-Date -Format yyyy.MM.dd_HHmm
    #$DTminusEinMonat = (get-date).AddDays(-30).ToString("yyy.MM.dd") 
    $ScriptPath = Get-ScriptDirectory 
    $ParentPath = (get-item $ScriptPath ).parent.FullName #Verzeichnissname wo dieser Skript liegt (eine Ebene zurück)
    $Projekt = (get-item $ScriptPath ).Name #nur Verzeichnissname
    

    write-host "`r`n"
    $FM = @(	
        @{Name = "Hostname:";               Value = "[$PC]" }
        , @{Name = "Date/Time:";            Value = "[$datum]" }
        #, @{Name = "Date -30 days:";	    Value = "[$DTminusEinMonat]" }
        , @{Name = "Script Pathfragment:";  Value = "[$Projekt]" }
        , @{Name = "Complete Path:";        Value = "[$ScriptPath]" }
        , @{Name = "Parent Path:";          Value = "[$ParentPath]" }
    )
    $FM | ForEach-Object { [PSCustomObject]$_ } | Format-Table -Property Name, Value -AutoSize

    
    start-countdown 10
}




################################################################################################################################
#### MAIN PROGRAMM ###########
$global:Modul = 'Main'

Clear-Host


#region get all files recursivly, and make backup
    SetDebugState($false)
    Write-debug "`r`n`r`n------------------------------------------------------`r`n"
    Write-debug 'get all files recursivly'
    
    $ScriptInPath = Get-ScriptDirectory #path where the script stored

    #region Get Backup-Folder from User-Input
        # $ScriptInPath = Get-FolderDialog ("$ScriptInPath")
        # if ($ScriptInPath -eq "-CANCEL-"){
        #     Write-Warning "No Folder selected - Exit Script"
        #     exit
        # }
        # elseif ($ScriptInPath -eq "-ERROR-") {
        #     Write-Error "Error in Get-Folder-Dialog - Exit Script"
        #     exit
        # }
    #endregion        

    $ProjectName = (get-item $ScriptInPath ).Name #only the Name of the Path
    
   #Region Check for BackupSettings.json and if it there fill the Variables
    $SettingsFile = "$ScriptInPath\BackupSettings.json"
    if (Test-Path($SettingsFile)){
        $json = (Get-Content $SettingsFile -Raw) | ConvertFrom-Json
        foreach ($var in $json.psobject.properties) {
                $value = $json.psobject.properties.Where({$_.name -eq $var.name}).value
                Set-Variable -Name $var.name -Value $value
            }

            
        #region SelfUpdate

        #Check if allowed and the version is les then PGM-Version
        if ($AllowUpdate){
            if ((Test-Path($UpdateFromPath + "\" + $UpdateFile)) -And (Test-Path($UpdateFromPath + "\" + $UpdateVersionFile))){
                $VersionJson = (Get-Content "$UpdateFromPath\$UpdateVersionFile" -Raw) | ConvertFrom-Json
                $NewestVersion = $VersionJson.psobject.properties.Where({$_.name -eq "CurrentVersion"}).value
                if ($UpdateVersion -lt $NewestVersion){
                    
                    #copy the File
                    copy-item "$UpdateFromPath\$UpdateFile" "$ScriptInPath\$UpdateFile" -force #-WhatIf
                    #Set New New Version in actual Settings-Json
                    $json.UpdateVersion = $NewestVersion
                    $json | ConvertTo-Json -depth 32| set-content $SettingsFile

                    Write-Warning "This script is updated now! Plese restart it again to perform your Backup"
                    pause
                    exit #exit this script

                }
            }
        }
    #endregion  
  
   }

   #endregion

    # 13.09.2022 Robocopy Region
        $ExcludePath = "/XD "
        $Excludes | ForEach-Object {
            $ExcludePath += '"' + $_ + '" '
        }
    # end Robocopy Region


    if ($TargetPaths -eq 0) {
        $TargetPaths[0] =  $ScriptInPath + $Prefix + '_' + $AktualDate + $Sufix #if target not Set -> save in Prevous Path
    }

    # 2022.09.14 viru Bugfix 
    $ProjectName = $ProjectName.Replace(":","").Replace("\","")
  
     
    if ($UseRobocopy){
        sectionY "use Robocopy..."
        Write-Host "Exclude This:[$ExcludePath]"
        foreach ( $TargetPath in $TargetPaths ){
            $TP = $TargetPath + "\" + $Prefix + $ProjectName + $Sufix + "\"
            $act = "robocopy $ScriptInPath $TP $Parameter $ExcludePath"
            log $act
             Write-Host "-->"
             Write-Host $act
            Invoke-Expression $act
        }    
    } else {
        sectionY "use Powershell"
        sectionY "collecting, please wait..."
        Write-Host "Exclude This Files / Folder:" + $Excludes
        Write-Host "Save there: $TargetPaths[0]"
        
        if ($CompressIntoTargetPaths){
        
            $TargetPaths | ForEach-Object{ 
                Write-Host "Copying:" $_
                $zd = $_ + '\'+ $Prefix + $ProjectName + $Sufix + '.7z'
                write-host "Save there: $zd"
                compress "$ScriptInPath" "$zd"
            }  
        }
        else{
            # path-Exclusion found on: https://stackoverflow.com/questions/20412043/powershell-where-statement-notcontains
            $TP = ''
            Get-ChildItem -Path $ScriptInPath -Recurse -Force |
                where-Object {$path = $_.fullname; -not @($Excludes | 
                    Where-Object {$path -like $_ -or $path -like "$_\*" }) } | 
                        ForEach-Object{
                        
                            foreach ( $TargetPath in $TargetPaths ){

                                $TP = $TargetPath + "\" + $Prefix + $ProjectName + $Sufix + "\"

                                try {
                                    if (! $_.PSIsContainer){     
                                            Write-Host "Copying:" $_.FullName " in :" $TP  -ForegroundColor Green
                                            copy-item $_.FullName -Destination $_.FullName.Replace($ScriptInPath, $TP) -Recurse -force # -WhatIf
                                        }                     
                                    else{
                                        Write-Host "Add Path:" $_.FullName.Replace($ScriptInPath , $TP) -ForegroundColor yellow
                                        Add-Path($_.FullName.Replace($ScriptInPath, $TP))
                                    }
                                }
                                catch { 
                                    Write-Warning "$global:Modul -  Something went wrong on $_" 
                                }
                            }
                        }
        }
    }    
#endregion



#$FilesObject | ConvertTo-Json
    Write-Host "`r`n`r`n"
    Write-Warning "------------------------------------------------------`r`n"
    Write-Warning 'Skript is done!'
    Write-Warning "When you don't see any red than is all fine ;-)"

#start-countdown 30
#pause



