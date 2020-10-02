!module showarr
!use, intrinsic :: iso_c_binding
!      use parm
!implicit none
!private
!public FlowRunDay
    
    
 !  contains    
  
   subroutine FlowRunDay(day,precip,sz) !bind(C,name = "FlowRunDay")
!DEC$ ATTRIBUTES DLLEXPORT :: FlowRunDay

    use parm
        implicit none
    !real(c_double), dimension(*),intent(in):: precip
    !real(c_double),intent(in),value :: day
   
  ! subroutine FlowRunDay(day, precip)

      integer :: idlst, j, iix, iiz, ic, mon, ii, k, ly, L, IDAP, IYP, day, sz
      real*8 :: xx
      integer :: eof
      integer, parameter :: DP = kind(0d0)
      real(kind=DP), dimension(sz) :: precip
      eof = 0

       
  !      do i = id1, idlst                            !! begin daily loop
          iida=day
          i=day
          !screen print days of the year for subdaily runs 
          if (ievent>0) then
            write(*,'(3x,I5,a6,i4)') iyr,'  day:', iida
          endif
         
          !!if last day of month 
          if (i_mo /= mo_chk) then
            immo = immo + 1
          endif

          !! initialize variables at beginning of day
          call sim_initday
          !! added for Srini in output.mgt nitrogen and phosphorus nutrients per JGA by gsm 9/8/2011
                  
          sol_sumno3 = 0.
          sol_sumsolp = 0.
          do j = 1, mhru
            do ly = 1, sol_nly(j)
              sol_sumno3(j) = sol_sumno3(j) + sol_no3(ly,j) + sol_nh3(ly,j)
              sol_sumsolp(j) = sol_sumsolp(j) + sol_solp(ly,j)
            enddo
          enddo
 
          if ( fcstyr == iyr .and. fcstday == i) then
            ffcst = 1
            pcpsim = 2
            tmpsim = 2
            rhsim = 2
            slrsim = 2
            wndsim = 2
            igen = igen + iscen
            call gcycl
            do j = 1, subtot
              ii = 0
              ii = fcst_reg(j)
              if (ii <= 0) ii = 1
              do mon = 1, 12
                 tmpmx(mon,j) = 0.
                 tmpmn(mon,j) = 0.
                 tmpstdmx(mon,j) = 0.
                 tmpstdmn(mon,j) = 0.
                 pcp_stat(mon,1,j) = 0.
                 pcp_stat(mon,2,j) = 0.
                 pcp_stat(mon,3,j) = 0.
                 pr_w(1,mon,j) = 0.
                 pr_w(2,mon,j) = 0.
                 tmpmx(mon,j) = ftmpmx(mon,ii)
                 tmpmn(mon,j) = ftmpmn(mon,ii)
                 tmpstdmx(mon,j) = ftmpstdmx(mon,ii)
                 tmpstdmn(mon,j) = ftmpstdmn(mon,ii)
                 pcp_stat(mon,1,j) = fpcp_stat(mon,1,ii)
                 pcp_stat(mon,2,j) = fpcp_stat(mon,2,ii)
                 pcp_stat(mon,3,j) = fpcp_stat(mon,3,ii)
                 pr_w(1,mon,j) = fpr_w(1,mon,ii)
                 pr_w(2,mon,j) = fpr_w(2,mon,ii)
              end do
            end do
          end if

          dtot = dtot + 1.
          nd_30 = nd_30 + 1
          if (nd_30 > 30) nd_30 = 1

          if (curyr > nyskip) ndmo(i_mo) = ndmo(i_mo) + 1

          if (pcpsim < 3) call clicon      !! read in/generate weather
          if (iatmodep == 2) then
            read (127,*,iostat=eof) iyp, idap, (rammo_d(l), rcn_d(l),drydep_nh4_d(l), drydep_no3_d(l),l=1, matmo)
        !     if (eof < 0) exit
          end if
          
          do k = 1, nhru
            subp(k) = precip(k)
          end do

           !! call resetlu
           if (ida_lup(no_lup) == i .and. iyr_lup(no_lup) == iyr) then
              call resetlu
              no_lup = no_lup + 1
           end if

          call command              !! command loop


        do ihru = 1, nhru                                    
          if (idaf > 180 .and. sub_lat(hru_sub(ihru)) < 0) then
            if (i == 180) then
               if (mgtop(nop(ihru),ihru) /=17) then         
                  dorm_flag = 1
                  call operatn
                  dorm_flag = 0
               endif
               nop(ihru) = nop(ihru) + 1
          
                if (nop(ihru) > nopmx(ihru)) then
                  nop(ihru) = 1
                end if
      
              phubase(ihru) = 0.
	        yr_skip(ihru) = 0
	      endif
	    
	    endif
        end do
        
	    !! write daily and/or monthly output
          if (curyr > nyskip) then
            call writed

          !! output.sol file
          if (isol == 1) call soil_write

            iida = i + 1
            call xmon
            call writem
          else
            iida = i + 1
            call xmon
          endif
          
           IF(ievent>0)THEN
              QHY(:,:,IHX(1))=0. 
              II=IHX(1)
              DO K=2,4
                  IHX(K-1)=IHX(K)
              END DO
              IHX(4)=II
          END IF
         

  !      end do                                        !! end daily loop

  
   end
 !  end module showarr